#include "yaz0.h"

uint32_t yaz0_rabin_karp(uint8_t* src, int srcSize, int srcPos, uint32_t* matchPos)
{
	int startPos, smp, i;
	uint32_t hash, curHash, curSize;
	uint32_t bestSize, bestPos;

	/* Set up some vars, start 0x1000 back from current position */
	smp = srcSize - srcPos;
	startPos = srcPos - 0x1000;
	bestPos = bestSize = 0;

	/* Check the size of the file minus the position in the file */
	/* If this is too small to consider encoding it, just skip it */
	if(smp < 3)
		return(0);

	/* We can only encode 0x111 bytes per chunk */
	if(smp > 0x111)
		smp = 0x111;

	/* Make sure we're reading from in bounds */
	if(startPos < 0)
		startPos = 0;

	/* Generate simple "hashes" by converting to a 3 byte int */
	/* Make a hash for where we're looking, and what we're matching */
	hash = *(int*)(src + srcPos);
	hash = hash >> 8;
	curHash = *(int*)(src + startPos);
	curHash = curHash >> 8;

	/* Search through data */
	/* This can probably benefit from SIMD */
	for(i = startPos; i < srcPos; i++)
	{
		/* If 3 bytes match, check for more */
		if(curHash == hash)
		{
			for(curSize = 3; curSize < smp; curSize++)
				if(src[i + curSize] != src[srcPos + curSize])
					break;

			/* Uodate best size if applicable */
			if(curSize > bestSize)
			{
				bestSize = curSize;
				bestPos = i;
				if(bestSize == 0x111)
					break;
			}
		}

		/* Scoot over 1 byte */
		curHash = (curHash << 8 | src[i + 3]) & 0x00FFFFFF;
	}

	/* Set match position, return the size of the match */
	*matchPos = bestPos;
	return(bestSize);
}

uint32_t yaz0_find_best(uint8_t* src, int srcSize, int srcPos, uint32_t* matchPos, uint32_t* pMatch, uint32_t* pSize, uint8_t* pFlag)
{
	int rv;

	/* Check to see if this location was found by a look-ahead */
	if(*pFlag == 1)
	{
		*pFlag = 0;
		return(*pSize);
	}

	/* Find best match */
	*pFlag = 0;
	rv = yaz0_rabin_karp(src, srcSize, srcPos, matchPos);

	/* The look-ahead part */
	if(rv >= 3)
	{
		/* Find best match if current one were to be a 1 byte copy */
		*pSize = yaz0_rabin_karp(src, srcSize, srcPos+1, pMatch);
		if(*pSize >= rv+2)
		{
			rv = *pFlag = 1;
			*matchPos = *pMatch;
		}
	}

	return(rv);
}

int yaz0_internal(uint8_t* src, int srcSize, uint8_t* dst)
{
	int dstPos, srcPos, codeBytePos;
	uint32_t numBytes, matchPos, dist, pMatch, pSize;
	uint8_t codeByte, bitmask, pFlag;

	srcPos = codeBytePos = 0;
	dstPos = codeBytePos + 1;
	bitmask = 0x80;
	codeByte = pFlag = 0;

	/* Go through all of src */
	while(srcPos < srcSize)
	{
		/* Try to find matching bytes for compressing */
		numBytes = yaz0_find_best(src, srcSize, srcPos, &matchPos, &pMatch, &pSize, &pFlag);

		/* Single byte copy */
		if(numBytes < 3)
		{
			dst[dstPos++] = src[srcPos++];
			codeByte |= bitmask;
		}

		/* Three byte encoding */
		else if(numBytes > 0x11)
		{
			dist = srcPos - matchPos - 1;

			/* Copy over 0R RR */
			dst[dstPos++] = dist >> 8;
			dst[dstPos++] = dist & 0xFF;

			/* Reduce N if needed, copy over NN */
			if(numBytes > 0x111)
				numBytes = 0x111;
			dst[dstPos++] = (numBytes - 0x12) & 0xFF;

			srcPos += numBytes;
		}

		/* Two byte encoding */
		else
		{
			dist = srcPos - matchPos - 1;

			/* Copy over NR RR */
			dst[dstPos++] = ((numBytes - 2) << 4) | (dist >> 8);
			dst[dstPos++] = dist & 0xFF;

			srcPos += numBytes;
		}

		/* Move bitmask to next byte */
		bitmask = bitmask >> 1;

		/* If all 8 bytes were used, write and move to the next one */
		if(bitmask == 0)
		{
			dst[codeBytePos] = codeByte;
			codeBytePos = dstPos;
			if(srcPos < srcSize)
				dstPos++;
			codeByte = 0;
			bitmask = 0x80;
		}
	}

	/* Copy over last byte if it hasn't already */
	if(bitmask != 0)
		dst[codeBytePos] = codeByte;

	/* Return size of dst */
	return(dstPos);
}

void yaz0_compress(uint8_t* src, int srcSize, uint8_t* dst, int* dstSize)
{
	int temp;

	/* Write the Yaz0 header */
	memcpy(dst, "Yaz0", 4);
	memcpy(dst + 4, &srcSize, 4);

	/* Encode, align dstSize to nearest multiple of 32 */
	temp = yaz0_internal(src, srcSize, dst + 16);
	*dstSize = (temp + 31) & -16;

	return;
}

/* This one might need a little work, it was written years ago */
/* It doesn't check bounds of source, and assumes dstSize is good */
/* Plus it's ugly and it currently doesn't work properly */
int yaz0_decompress(uint8_t* src, uint8_t** dst)
{
	uint32_t srcPlace, dstPlace, dstSize;
	uint32_t i, dist, copyPlace, numBytes;
	uint8_t codeByte, byte1, byte2, bitCount;

	dstSize = *(int*)(src + 4);
	*dst = malloc(dstSize);

	srcPlace = 0x10;
	dstPlace = bitCount = 0;
	while(dstPlace < dstSize)
	{
		/* If there are no more bits to test, get a new byte */
		if(!bitCount)
		{
			codeByte = src[srcPlace++];
			bitCount = 8;
		}

		/* If bit 7 is a 1, just copy 1 byte from source to destination */
		/* Else do some decoding */
		if(codeByte & 0x80)
		{
			(*dst)[dstPlace++] = src[srcPlace++];
		}
		else
		{
			/* Get 2 bytes from source */
			byte1 = src[srcPlace++];
			byte2 = src[srcPlace++];

			/* Calculate distance to move in destination */
			/* And the number of bytes to copy */
			dist = ((byte1 & 0xF) << 8) | byte2;
			copyPlace = dstPlace - (dist + 1);
			numBytes = byte1 >> 4;

			/* Do more calculations on the number of bytes to copy */
			if(!numBytes)
				numBytes = src[srcPlace++] + 0x12;
			else
				numBytes += 2;

			/* Copy data from a previous point in destination */
			/* to current point in destination */
			for(i = 0; i < numBytes; i++)
				(*dst)[dstPlace++] = (*dst)[copyPlace++];
		}

		/* Set up for the next read cycle */
		codeByte = codeByte << 1;
		bitCount--;
	}

	return(dstSize);
}
