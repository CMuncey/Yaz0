#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "yaz0.h"

int main(int argc, char** argv)
{
	uint8_t* src, *dst;
	int32_t srcSize, dstSize;
	char* fname;
	FILE* f;

	/* Argument check */
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s [inputFile] <outputFile>\n", argv[0]);
		return(1);
	}

	/* Create output file name if needed */
	if(argc < 3)
	{
		fname = malloc(strlen(argv[1]) + 5);
		strcpy(fname, argv[1]);
		strcat(fname, ".yaz");
	}
	else
	{
		fname = argv[2];
	}

	/* Make sure the output file will work before compressing */
	f = fopen(fname, "wb");
	if(f == NULL)
	{
		perror(fname);
		return(1);
	}
	fclose(f);

	/* Open input file, make sure it opened, get the size */
	f = fopen(argv[1], "rb");
	if(f == NULL)
	{
		perror(argv[1]);
		return(1);
	}
	fseek(f, 0, SEEK_END);
	srcSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	/* Calculate dstSize. Worst case scenario it's totally incompressible */
	/* In this case, every 8 bytes of input will need 1 more byte of output */
	/* Also need room for the yaz0 header, 16 bytes */
	dstSize = srcSize + (srcSize >> 3) + 16;

	/* Allocate space and read in the source file */
	src = malloc(srcSize);
	dst = malloc(dstSize);
	fread(src, 1, srcSize, f);
	fclose(f);

	/* Compress the source file */
	dstSize = yaz0_compress(src, srcSize, dst);
	free(src);

	/* Write the compressed file */
	printf("Compressed %d input bytes to %d output bytes\n", srcSize, dstSize);
	f = fopen(fname, "wb");
	fwrite(dst, 1, dstSize, f);
	fclose(f);
	free(dst);

	if(argc < 3)
		free(fname);

	return(0);
}

