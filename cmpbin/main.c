/*
 * Copyright (C) 2019 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <malloc.h>
#include <lzma.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

static bool init_encoder(lzma_stream *strm)
{
	lzma_options_lzma opt_lzma2;
	if (lzma_lzma_preset(&opt_lzma2, LZMA_PRESET_DEFAULT))
		return false;

	lzma_filter filters[] = {
		{ .id = LZMA_FILTER_LZMA2, .options = &opt_lzma2 },
		{ .id = LZMA_VLI_UNKNOWN, .options = NULL },
	};
	lzma_ret ret = lzma_stream_encoder(strm, filters, LZMA_CHECK_NONE);

	if (ret == LZMA_OK)
		return true;

	return false;
}

int cmpSize = 0;

static bool compress(lzma_stream *strm, FILE *infile, FILE *outfile)
{
	lzma_action action = LZMA_RUN;

	uint8_t inbuf[BUFSIZ];
	uint8_t outbuf[BUFSIZ];

	strm->next_in = NULL;
	strm->avail_in = 0;
	strm->next_out = outbuf;
	strm->avail_out = sizeof(outbuf);

	while (true) {
		if (strm->avail_in == 0 && !feof(infile)) {
			strm->next_in = inbuf;
			strm->avail_in = fread(inbuf, 1, sizeof(inbuf),
					infile);

			if (ferror(infile)) {
				fprintf(stderr, "Read error: %s\n",
						strerror(errno));
				return false;
			}

			if (feof(infile))
				action = LZMA_FINISH;
		}

		lzma_ret ret = lzma_code(strm, action);

		if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
			size_t write_size = sizeof(outbuf) - strm->avail_out;
			if (fwrite(outbuf, 1, write_size, outfile)
					!= write_size) {
				fprintf(stderr, "Write error: %s\n",
						strerror(errno));
				return false;
			}
			cmpSize += write_size;

			strm->next_out = outbuf;
			strm->avail_out = sizeof(outbuf);
		}

		if (ret != LZMA_OK) {
			if (ret == LZMA_STREAM_END)
				return true;
			return false;
		}
	}
}

int main(int argc, char *argv[])
{
	FILE *inF = fopen("emu_decmp.bin","rb");
	if(!inF)
	{
		printf("Failed to read emu_decmp.bin!\n");
		return -1;
	}
	FILE *outF = fopen("emu_cmp.bin","wb");
	if(!outF)
	{
		fclose(inF);
		printf("Failed to write to emu_cmp.bin!\n");
		return -2;
	}
	fwrite(&cmpSize,1,4,outF);
	//compress start
	printf("Compressing...\n");
	lzma_stream strm = LZMA_STREAM_INIT;
	bool success = init_encoder(&strm);
	if (success)
		success = compress(&strm, inF, outF);
	if(!success)
	{
		printf("Failed to compress!\n");
		fclose(outF);
		fclose(inF);
		return -3;
	}
	fclose(inF);

	if(cmpSize > 0x39FC)
		printf("WARNING: This file wont fit into dedicated RAM! %i bytes instead of %i!\n", cmpSize, 0x39FC);

	//write back compressed size
	cmpSize = __builtin_bswap32(cmpSize);
	fseek(outF,0,SEEK_SET);
	fwrite(&cmpSize,1,4,outF);

	fclose(outF);
	printf("Done!\n");

	return 0;
}
