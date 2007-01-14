#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>

#include "fileio.h"
#include "zipfile.h"
#include "kunzip.h"
#include "../strbuf.h"
#include "../mem.h"

/*

This code is Copyright 2005-2006 by Michael Kohn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2 as published by the Free Software Foundation

*/

#define BUFFER_SIZE 16738

/* #define _GNU_SOURCE */

unsigned int copy_file_tobuf(FILE *in, STRBUF *out, int len)
{
	unsigned char buffer[BUFFER_SIZE];
	uLong checksum;
	int t, r;

	checksum = crc32(0L, Z_NULL, 0);

	t = 0;

	while (t < len) {
		if (t + BUFFER_SIZE < len) {
			r = BUFFER_SIZE;
		} else {
			r = len - t;
		}

		read_buffer(in, buffer, r);
		strbuf_append_n(out, (char *)buffer, r);
		checksum = crc32(checksum, buffer, r);
		t = t + r;
	}

	return checksum;
}

int read_zip_header(FILE *in,
		    struct zip_local_file_header_t *local_file_header)
{
	local_file_header->signature = read_int(in);
	if (local_file_header->signature != 0x04034b50)
		return -1;

	local_file_header->version = read_word(in);
	local_file_header->general_purpose_bit_flag = read_word(in);
	local_file_header->compression_method = read_word(in);
	local_file_header->last_mod_file_time = read_word(in);
	local_file_header->last_mod_file_date = read_word(in);
	local_file_header->crc_32 = read_int(in);
	local_file_header->compressed_size = read_int(in);
	local_file_header->uncompressed_size = read_int(in);
	local_file_header->file_name_length = read_word(in);
	local_file_header->extra_field_length = read_word(in);
	local_file_header->descriptor_length = 0;

	/* if the 4th bit in the general_purpose_bit_flag is set,
	   crc32, compressed_size and uncompressed_size are written
	   into a data descriptor that follows the compressed
	   data */
	if (local_file_header->general_purpose_bit_flag & 8) {
		long data_start = ftell(in);
		unsigned int signature;

		while (1) {
			signature = read_int(in);

			if (feof(in)) {
				fseek(in, data_start, SEEK_SET);
				return -1;
			}

			if (signature == 0x08074b50) {
				local_file_header->crc_32 = read_int(in);
				local_file_header->compressed_size =
					read_int(in);
				local_file_header->uncompressed_size =
					read_int(in);
				local_file_header->descriptor_length = 16;
				fseek(in, data_start, SEEK_SET);
				return 0;
			}

			fseek(in, -3, SEEK_CUR);
		}

	}
	return 0;
}

#ifdef DEBUG
int print_zip_header(struct zip_local_file_header_t *local_file_header)
{
	printf("ZIP LOCAL FILE HEADER\n");
	printf("----------------------------------\n");
	printf("Signature: %02x%02x%02x%02x\n",
	       ((local_file_header->signature >> 24) & 255),
	       ((local_file_header->signature >> 16) & 255),
	       ((local_file_header->signature >> 8) & 255),
	       (local_file_header->signature & 255));
	printf("Version: %d\n", local_file_header->version);
	printf("General Purpose Bit Flag: %d\n",
	       local_file_header->general_purpose_bit_flag);
	printf("Compression Method: %d\n",
	       local_file_header->compression_method);
	printf("Last Mod File Time: %d\n",
	       local_file_header->last_mod_file_time);
	printf("Last Mod File Date: %d\n",
	       local_file_header->last_mod_file_date);
	printf("CRC-32: %d\n", local_file_header->crc_32);
	printf("Compressed Size: %d\n", local_file_header->compressed_size);
	printf("Uncompressed Size: %d\n", local_file_header->uncompressed_size);
	printf("File Name Length: %d\n", local_file_header->file_name_length);
	printf("Extra Field Length: %d\n",
	       local_file_header->extra_field_length);
	printf("File Name: %s\n", local_file_header->file_name);

	return 0;
}
#endif

STRBUF *kunzip_file_tobuf(FILE *in)
{
	STRBUF *out;
	struct zip_local_file_header_t local_file_header;
	int ret_code;
	int checksum;
	long marker;

	ret_code = 0;

	if (read_zip_header(in, &local_file_header) == -1)
		return NULL;

	local_file_header.file_name =
		(char *)ymalloc(local_file_header.file_name_length + 1);
	local_file_header.extra_field =
		(unsigned char *)ymalloc(local_file_header.extra_field_length + 1);

	read_chars(in, local_file_header.file_name,
		   local_file_header.file_name_length);
	read_chars(in, (char *)local_file_header.extra_field,
		   local_file_header.extra_field_length);

	marker = ftell(in);

#ifdef DEBUG
	print_zip_header(&local_file_header);
#endif

	out = strbuf_new();

	if (local_file_header.compression_method == 0) {
		checksum =
			copy_file_tobuf(in, out,
					local_file_header.uncompressed_size);
	} else if (local_file_header.compression_method == Z_DEFLATED) {
		(void)strbuf_append_inflate(out, in);
		checksum = strbuf_crc32(out);
	} else {
		fprintf(stderr, "Unknown compression method\n");
		exit(EXIT_FAILURE);
	}

	if ((unsigned int)checksum != local_file_header.crc_32
	    && local_file_header.crc_32 != 0) {
		fprintf(stderr,
			"Warning: Checksum does not match: %d %d.\nPossibly the file"
			" is corrupted otr truncated.\n", checksum,
			local_file_header.crc_32);
		ret_code = -4;
	}

	yfree(local_file_header.file_name);
	yfree(local_file_header.extra_field);

	fseek(in, marker + local_file_header.compressed_size, SEEK_SET);

	if ((local_file_header.general_purpose_bit_flag & 8) != 0) {
		read_int(in);
		read_int(in);
		read_int(in);
	}

	return out;
}

STRBUF *kunzip_next_tobuf(char *zip_filename, int offset)
{
	FILE *in;
	STRBUF *buf;
	long marker;

	in = fopen(zip_filename, "rb");
	if (in == 0) {
		return NULL;
	}

	fseek(in, offset, SEEK_SET);

	buf = kunzip_file_tobuf(in);
	marker = ftell(in);
	fclose(in);

	return buf;
}

/*
  Match Flags:
  bit 0: set to 1 if it should be exact filename match
  set to 0 if the archived filename only needs to contain that word
  bit 1: set to 1 if it should be case sensitive
  set to 0 if it should be case insensitive
*/

int kunzip_get_offset_by_name(char *zip_filename, char *compressed_filename,
			      int match_flags, int skip_offset)
{
	FILE *in;
	struct zip_local_file_header_t local_file_header;
	int i = 0, curr;
	char *name = 0;
	int name_size = 0;
	long marker;

	in = fopen(zip_filename, "rb");
	if (in == 0) {
		return -1;
	}

	if (skip_offset != -1) {
		fseek(in, skip_offset, SEEK_SET);
	}

	while (1) {
		curr = ftell(in);
		i = read_zip_header(in, &local_file_header);
		if (i == -1)
			break;

		if (skip_offset < 0 || curr > skip_offset) {
			marker = ftell(in);	/* nasty code.. please make it nice later */

			if (name_size < local_file_header.file_name_length + 1) {
				if (name_size != 0)
					yfree(name);
				name_size =
					local_file_header.file_name_length + 1;
				name = ymalloc(name_size);
			}

			read_chars(in, name,
				   local_file_header.file_name_length);
			name[local_file_header.file_name_length] = 0;

			fseek(in, marker, SEEK_SET);	/* and part 2 of nasty code */

			if ((match_flags & 1) == 1) {
				if (strcmp(compressed_filename, name) == 0)
					break;
			} else {
				if (strstr(name, compressed_filename) != 0)
					break;
			}
		}

		fseek(in, local_file_header.compressed_size +
		      local_file_header.file_name_length +
		      local_file_header.extra_field_length +
		      local_file_header.descriptor_length, SEEK_CUR);

	}

	if (name_size != 0)
		yfree(name);

	fclose(in);

	if (i != -1) {
		return curr;
	} else {
		return -1;
	}
}
