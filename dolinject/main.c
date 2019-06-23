/*
 * Copyright (C) 2019 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <malloc.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include "../decmp/decmp.h"
#include "../emu/emu_cmp.h"

static const uint8_t magic[8] = { 0x00, 0x00, 0x00, 0x00, 0xC2, 0x33, 0x9F, 0x3D };
static const uint8_t dolcheck1[8] = { 0x38, 0x00, 0x00, 0x01, 0x98, 0x0D, 0x81, 0x81 };
static const uint8_t dolcheck2[8] = { 0x83, 0xEB, 0xFF, 0xFC, 0x7D, 0x61, 0x5B, 0x78 };
static const uint8_t dolcheck3[8] = { 0x38, 0x00, 0x00, 0xC4, 0xB0, 0x0B, 0x00, 0x04 };
static const uint32_t dolpatchoffset1 = 0x111EC;
static const uint32_t dolpatchoffset2 = 0x38CC8;
static const uint32_t dolpatchoffset3 = 0xA5DC;
static const uint8_t arraycheck[8] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x79, 0x46 };
static const uint32_t arraydstoffset = 0xB9220;
static const uint32_t arraysrcoffset = 0xBA260;
static const uint32_t arrayptr1 = 0xACCB6;
static const uint32_t arrayptr2 = 0xACD4A;
static const uint8_t endcheck[8] = { 0xB7, 0x9B, 0xAE, 0x5F, 0x37, 0xD9, 0x53, 0x84 };
static const uint32_t dollen = 0xD81F4;

static const uint32_t decmpoffset = 0xBA220;
static const uint32_t cmpemuoffset = 0xC8358;

void waitforexit()
{
	puts("Press enter to exit");
	getc(stdin);
}

int main(int argc, char *argv[])
{
	printf("Mega Man Anniversary Collection NES Audio DOL Patcher v0.6 by FIX94\n");
	if(decmp_size > 0x2080)
	{
		printf("Internal executable check 1 failed! decmp_size is %i bytes!\n", decmp_size);
		waitforexit();
		return -1;
	}
	if(emu_cmp_size > 0x3A00)
	{
		printf("Internal executable check 2 failed! emu_cmp_size is %i bytes!\n", emu_cmp_size);
		waitforexit();
		return -2;
	}

	if(argc < 2)
	{
		printf("Usage: dolinject.exe your.iso\n");
		waitforexit();
		return 0;
	}

	FILE *f = fopen(argv[1], "rb+");
	if(!f)
	{
		printf("ERROR: Unable to open %s with read/write access!\n", argv[1]);
		waitforexit();
		return -3;
	}
	uint8_t checkbuf[8];
	fseek(f, 0x18, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, magic, 8) != 0)
	{
		printf("ERROR: ISO magic not found! This may not be a valid GC ISO file.\n");
		fclose(f);
		waitforexit();
		return -4;
	}
	fseek(f, 0x420, SEEK_SET);
	uint32_t offset;
	fread(&offset, 1, 4, f);
	offset = __builtin_bswap32(offset);
	printf("DOL offset: %08x, sanity checking main hooks\n", offset);
	fseek(f, offset+dolpatchoffset1-8, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, dolcheck1, 8) != 0)
	{
		printf("ERROR: Sanity check 1 failed! DOL Unexpected\n");
		fclose(f);
		waitforexit();
		return -5;
	}

	fseek(f, offset+dolpatchoffset2-8, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, dolcheck2, 8) != 0)
	{
		printf("ERROR: Sanity check 2 failed! DOL Unexpected\n");
		fclose(f);
		waitforexit();
		return -6;
	}

	fseek(f, offset+dolpatchoffset3-8, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, dolcheck3, 8) != 0)
	{
		printf("ERROR: Sanity check 3 failed! DOL Unexpected\n");
		fclose(f);
		waitforexit();
		return -7;
	}

	bool array_needs_moving = false;
	printf("sanity checking decompressor space\n");

	fseek(f, offset+arraydstoffset, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, arraycheck, 8) != 0)
	{
		array_needs_moving = true;
		printf("Space needs to be cleared, making sure thats possible\n");
		fseek(f, offset+arraysrcoffset, SEEK_SET);
		fread(checkbuf, 1, 8, f);
		if(memcmp(checkbuf, arraycheck, 8) != 0)
		{
			printf("ERROR: Source array not found! DOL Unexpected!\n");
			fclose(f);
			waitforexit();
			return -8;
		}
	}
	else
		printf("Space already cleared\n");

	printf("sanity checking end of DOL can be accessed\n");
	fseek(f, offset+dollen-8, SEEK_SET);
	fread(checkbuf, 1, 8, f);
	if(memcmp(checkbuf, endcheck, 8) != 0)
	{
		printf("ERROR Unable to read end of DOL File!\n");
		fclose(f);
		waitforexit();
		return -9;
	}

	printf("OK so far, reading in whole DOL file and patching it\n");
	fseek(f, offset, SEEK_SET);
	uint8_t *buf = malloc(dollen);
	if(!buf)
	{
		printf("ERROR: Unable to allocate space for DOL!\n");
		fclose(f);
		waitforexit();
		return -10;
	}
	fread(buf, 1, dollen, f);

	//patch address jump 1
	buf[dolpatchoffset1+0] = 0x48;
	buf[dolpatchoffset1+1] = 0x0A;
	buf[dolpatchoffset1+2] = 0x90;
	buf[dolpatchoffset1+3] = 0x35;

	//patch address jump 2
	buf[dolpatchoffset2+0] = 0x48;
	buf[dolpatchoffset2+1] = 0x08;
	buf[dolpatchoffset2+2] = 0x15;
	buf[dolpatchoffset2+3] = 0x5C;

	//patch address jump 3
	buf[dolpatchoffset3+0] = 0x48;
	buf[dolpatchoffset3+1] = 0x0A;
	buf[dolpatchoffset3+2] = 0xFC;
	buf[dolpatchoffset3+3] = 0x4C;

	if(array_needs_moving)
	{
		//patch array ptr 1
		buf[arrayptr1+0] = 0xC2;
		buf[arrayptr1+1] = 0x20;

		//patch array ptr 2
		buf[arrayptr2+0] = 0xC2;
		buf[arrayptr2+1] = 0x20;

		//copy array out of our used region
		memcpy(buf+arraydstoffset, buf+arraysrcoffset, 0x1000);
	}

	//copy decompressor into space
	memset(buf+decmpoffset, 0, 0x2080);
	memcpy(buf+decmpoffset, decmp, decmp_size);

	//copy compressed emu into space
	memset(buf+cmpemuoffset, 0, 0x3A00);
	memcpy(buf+cmpemuoffset, emu_cmp, emu_cmp_size);

	printf("Writing back patched DOL\n");
	fseek(f, offset, SEEK_SET);
	fwrite(buf, 1, dollen, f);
	fclose(f);
	free(buf);
	printf("All done!\n");

	waitforexit();
	return 0;
}
