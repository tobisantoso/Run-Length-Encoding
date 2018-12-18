//Tobisantoso
//https://github.com/tobisantoso
//github.com/tobisantoso

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma pack(push,1)
typedef struct {
	char         bfType[2];        
	unsigned int bfSize;	   
	short        bfReserved1;  
	short        bfReserved2;  
	unsigned int bfOffBits;     
} file_header;

typedef struct {
	file_header  fileheader;
	unsigned int biSize;  
	int          biWidth;  
	int          biHeight; 
	short        biPlanes; 
	short        biBitCount; 
	unsigned int biCompression; 
	unsigned int biSizeImage;   
	int          biXPelsPerMeter; 
	int          biYPelsPerMeter;
	unsigned int biClrUsed; 
	unsigned int biClrImportant;  
} info_header;

#pragma pack(pop)

typedef struct {
	info_header* pInfo;
	file_header pFile;
	unsigned char *data;
	unsigned char* colorTableData;
} bitmap;

int foo(char* input, char *output);
int readBitmap(char* name, bitmap* bm);
int writeBitmap(char* name, bitmap* bm);
int compressM(bitmap* bm);

int foo(char* input, char *output) {

	bitmap* bmUn = (bitmap*)malloc(sizeof(bitmap));
	readBitmap(input, bmUn);
	int res = compressM(bmUn);
	writeBitmap(output, bmUn);
	free(bmUn->pInfo);
	free(bmUn->colorTableData);
	free(bmUn->data);
	free(bmUn);
	return res;
}

int compressM(bitmap* bm) {
	int imageSize = bm->pInfo->biSizeImage;
	int width = bm->pInfo->biWidth;
	unsigned char* result = (unsigned char*)malloc(sizeof(unsigned char)*imageSize);

	int i;
	// pixels
	unsigned char current = bm->data[0];
	unsigned char last = '0';

	int run = 0; 
	int resultIndex = 0; 
	int isAbsoluteMode = 0; 
	int absoluteStartIndex = 0; 
	int absoluteRun = 0; 
	int forceNormal = 0; 

	for (i = 1; i < imageSize; i++) {
		last = current;
		current = bm->data[i];

		run += 1;
		if (isAbsoluteMode) {
			absoluteRun += 1;

			if (i%width == 0) {
				if (absoluteRun > 2) {
					result[absoluteStartIndex + 1] = absoluteRun;
					result[resultIndex++] = last;
					if (absoluteRun % 2 == 1) {
						result[resultIndex++] = 0;
					}
				}
				else {
					result[resultIndex - 3] = 1;
					result[resultIndex - 2] = result[resultIndex - 1];
					result[resultIndex - 1] = 1;
					result[resultIndex++] = last;
				}

				result[resultIndex++] = 0;
				result[resultIndex++] = 0;
				absoluteRun = 0;
				run = 0;
				isAbsoluteMode = 0;
				continue;
			}

			if (last == current) {
				if (run > 1) {
					if (absoluteRun - run < 3) {
						forceNormal = 1;
						resultIndex = absoluteStartIndex;
						i -= absoluteRun;
						current = bm->data[i];
					}
					else {
						result[absoluteStartIndex + 1] = absoluteRun - run;
						resultIndex -= 1;
						i -= 2;
						if ((absoluteRun - run) % 2 == 1) {
							result[resultIndex++] = 0;
						}
					}
					run = 0;
					absoluteRun = 0;
					isAbsoluteMode = 0;
				}
				else {
					result[resultIndex++] = last;
				}
			}
			else {
				result[resultIndex++] = last;
				run = 0;
			}

			if (absoluteRun == 255) {
				result[absoluteStartIndex + 1] = absoluteRun;
				absoluteRun = 0;
				isAbsoluteMode = 0;
				run = 0;
			}
		}
		else { 
			if (i%width == 0) {
				result[resultIndex++] = run;
				result[resultIndex++] = last;
				result[resultIndex++] = 0;
				result[resultIndex++] = 0;
				run = 0;
				forceNormal = 0;
				continue;
			}

			if (last != current) { 
								 
				if (run < 3 && !forceNormal) {
					isAbsoluteMode = 1;
					absoluteStartIndex = resultIndex;
					absoluteRun = run;
					result[absoluteStartIndex] = 0;
					resultIndex += 2;
					result[resultIndex++] = last;
					if (run == 2) {
						result[resultIndex++] = last;
					}
				}
				else {
					result[resultIndex++] = run;
					result[resultIndex++] = last;
					forceNormal = 0;
				}
				run = 0;
			}

			if (run == 255) {
				result[resultIndex++] = run;
				result[resultIndex++] = last;
				forceNormal = 0;
				run = 0;
			}
		}
	}
	if (!isAbsoluteMode) {
		result[resultIndex++] = run + 1;
		result[resultIndex++] = last;
	}
	else {
		if (absoluteRun > 1) {
			result[resultIndex++] = current;
			result[absoluteStartIndex + 1] = absoluteRun + 1;
		}
		else {
			result[resultIndex++] = 1;
			result[resultIndex++] = last;

			result[resultIndex++] = 1;
			result[resultIndex++] = current;
		}
	}

	result[resultIndex++] = 0;
	result[resultIndex++] = 1;

	free(bm->data);
	bm->data = (unsigned char*)malloc(sizeof(unsigned char)*resultIndex);
	bm->data = result;

	bm->pInfo->biCompression = 1;
	bm->pInfo->biSizeImage = resultIndex;
	bm->pFile.bfSize = resultIndex + bm->pFile.bfOffBits;

	return 0;
}

int readBitmap(char* name, bitmap* bm) {

	int n;
#pragma warning (disable : 4996)
	FILE* fp = fopen(name, "rb");
	if (fp == NULL) {
		printf("File not found");
		return -1;
	}

	bm->pInfo = (info_header*)malloc(sizeof(info_header));
	if (bm->pInfo == NULL)
		return -1;

	n = fread(bm->pInfo, sizeof(info_header), 1, fp);
	if (n < 1) {
		printf("Header tidak bisa");
		return -1;
	}

	bm->pFile = bm->pInfo->fileheader;
	int structSize = bm->pInfo->biSize;
	int colorTableSize = bm->pFile.bfOffBits - structSize;

	bm->colorTableData = (unsigned char*)malloc(sizeof(unsigned char) * colorTableSize);

	fseek(fp, structSize, SEEK_SET);
	n = fread(bm->colorTableData, sizeof(unsigned char), colorTableSize, fp);
	if (n < 1) {
		return -1;
	}

	bm->data = (unsigned char*)malloc(sizeof(unsigned char)*bm->pInfo->biSizeImage);
	if (bm->data == NULL) {
		return -1;
	}

	fseek(fp, sizeof(unsigned char)*bm->pInfo->fileheader.bfOffBits, SEEK_SET);

	n = fread(bm->data, sizeof(unsigned char), bm->pInfo->biSizeImage, fp);
	if (n < 1) {
		return -1;
	}

	fclose(fp);
	return 0;
}

int writeBitmap(char* name, bitmap* bm) {
	int n;
#pragma warning (disable : 4996)
	FILE* out = fopen(name, "wb");
	if (out == NULL) {
		return -1;
	}
	n = fwrite(bm->pInfo, sizeof(unsigned char), sizeof(info_header), out);
	if (n < 1) {
		return -1;
	}
	int structSize = bm->pInfo->biSize;
	int colorTableSize = bm->pFile.bfOffBits - structSize;

	fseek(out, structSize, SEEK_SET);
	n = fwrite(bm->colorTableData, sizeof(unsigned char), colorTableSize, out);
	if (n < 1) {
		return -1;
	}

	fseek(out, sizeof(unsigned char)*bm->pInfo->fileheader.bfOffBits, SEEK_SET);
	n = fwrite(bm->data, sizeof(unsigned char), bm->pInfo->biSizeImage, out);
	if (n < 1) {
		return -1;
	}

	fclose(out);

}
int main(int argc, char **argv) {

	char* input = "tiger.bmp";
	char* output = "tiger2.bmp";

	if (argc > 1) {
		int i;
		float n = atoi(argv[1]) * 1.0;
		float sumSeconds = 0.0;
		for (i = 0; i < n; i++) {
			clock_t start = clock();
			foo(input, output);
			clock_t end = clock();


			float seconds = (float)(end - start) / CLOCKS_PER_SEC;
			sumSeconds = sumSeconds + seconds;
		}
		printf("Average Runtime: %f seconds", sumSeconds / n);
		printf("\n");
	}
	else {
		printf("Usage: %s MODE [args...]\n", argv[0]);
		printf("%s", argv[0]);
	}
	return 0;

}
