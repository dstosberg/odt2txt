/*
 * odt2txt.c: A simple (and stupid) converter from OpenDocument Text
 *            to plain text.
 *
 * Copyright (c) 2006 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem.h"
#include "stringops.h"
#include "kunzip/kunzip.h"

static int opt_raw;
static char *opt_encoding;
static int opt_width = 63;
static const char *opt_filename;

#define BUF_SZ 4096

#ifndef ICONV_CHAR
#define ICONV_CHAR char
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct subst {
	char *utf;
	char *ascii;
};

static void usage(void)
{
	printf("Syntax:   odt2txt [options] filename\n\n"
	       "Converts an OpenDocument Text to raw text.\n\n"
	       "Options:  --raw         Print raw XML\n"
	       "          --encoding=X  Do not try to autodetect the terminal encoding, but\n"
	       "                        convert the document to encoding X unconditionally\n"
	       "          --width=X     Wrap text lines after X characters. Default: 65.\n\n");
	exit(EXIT_FAILURE);
}

static const char *create_tmpdir()
{
	char *dirnam;
	char *tmpdir;
	char *pid;
	size_t pidlen;
	size_t left = PATH_MAX;
	int r;

	pid = ymalloc(20);
	pidlen = snprintf(pid, 20,  "%d", (int)getpid());
	if (pidlen >= 20 || pidlen <= 0) {
		fprintf(stderr, "Couldn't get pid\n");
		exit(EXIT_FAILURE);
	}

	tmpdir = getenv("TMPDIR");
	if (!tmpdir)
		tmpdir = "/tmp";

	dirnam = ymalloc(left);
	*dirnam = '\0';
	strlcat(dirnam, tmpdir, left);
	strlcat(dirnam, "/", left);
	strlcat(dirnam, "odt2txt-", left);
	strlcat(dirnam, pid, left);
	yfree(pid);
	left = strlcat(dirnam, "/", left);

	if (left >= PATH_MAX) {
		fprintf(stderr, "path too long");
		exit(1);
	}

	r = mkdir(dirnam, S_IRWXU);
	if (r != 0) {
		fprintf(stderr, "Cannot create %s: %s", dirnam, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return(dirnam);
}

static void yrealloc_buf(char **buf, char **mark, size_t len) {
	ptrdiff_t offset = *mark - *buf;
	*buf = yrealloc(*buf, len);
	*mark = *buf + offset;
}

static char *conv(char *docbuf)
{
	iconv_t ic;
	ICONV_CHAR *doc;
	char *out, *outbuf;
	size_t inleft, outleft = 0;
	size_t conv;
	size_t outlen = 0;
	const size_t alloc_step = 4096;

	char *input_enc = "utf-8";
	ic = iconv_open(opt_encoding, input_enc);
	if (ic == (iconv_t)-1) {
		if (errno == EINVAL) {
			fprintf(stderr, "warning: Conversion from %s to %s is not supported.\n",
				input_enc, opt_encoding);
			ic = iconv_open("us-ascii", input_enc);
			if (ic == (iconv_t)-1) {
				exit(1);
			}
			fprintf(stderr, "warning: Using us-ascii as fall-back.\n");
		} else {
			fprintf(stderr, "iconv_open returned: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	inleft = strlen(docbuf);
	doc = (ICONV_CHAR*)docbuf;
	outlen = alloc_step; outleft = alloc_step;
	outbuf = ymalloc(alloc_step);
	out = outbuf;
	outleft = alloc_step;

	do {
		if (!outleft) {
			outlen += alloc_step; outleft += alloc_step;
			yrealloc_buf(&outbuf, &out, outlen);
		}
		conv = iconv(ic, &doc, &inleft, &out, &outleft);
		if (conv == (size_t)-1) {
			if(errno == E2BIG) {
				continue;
			} else if ((errno == EILSEQ) || (errno == EINVAL)) {
				char skip = 1;

				if ((unsigned char)*doc > 0x80)
					skip++;
				if ((unsigned char)*doc > 0xDF)
					skip++;
				if ((unsigned char)*doc > 0xF0)
					skip++;

				doc += skip;
				*out = '?';
				out++;
				outleft--;
				inleft--;
				continue;
			}
			fprintf(stderr, "iconv returned: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	} while(inleft != 0);

	if (!outleft) {
		outbuf = yrealloc(outbuf, outlen + 1);
	}
	*out = '\0';

	if(iconv_close(ic) == -1) {
		fprintf(stderr, "iconv_close returned: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return outbuf;
}

static const char *unzip_doc(const char *filename, const char *tmpdir)
{
	int r;
	char *content_file;
	struct stat st;

	kunzip_inflate_init();
	r = kunzip_get_offset_by_name((char*)filename, "content.xml", 3, -1);

	if(!r) {
		fprintf(stderr,
			"Can't open %s: Is it an OpenDocument Text?\n", filename);
		exit(EXIT_FAILURE);
	}

	r = kunzip_next((char*)filename, (char*)tmpdir, r);
	kunzip_inflate_free();

	content_file = ymalloc(PATH_MAX);
	*content_file = '\0';
	strlcpy(content_file, tmpdir, PATH_MAX);
	r = strlcat(content_file, "content.xml", PATH_MAX);

	if (r >= PATH_MAX) {
		fprintf(stderr, "unzip_doc: name too long\n");
		exit(EXIT_FAILURE);
	}

	if (0 != stat(content_file, &st)) {
		fprintf(stderr, "Unzipping file failed. %s does not exist: %s\n",
			content_file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return(content_file);
}

static size_t read_doc(char **buf, const char *filename)
{
	int fd;
	char *bufp;
	size_t fpos = 0;
	size_t buf_sz = BUF_SZ;

	*buf  = ymalloc(buf_sz);
	bufp = *buf;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Can't open %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (1) {
		size_t read_sz;

		read_sz = read(fd, bufp, BUF_SZ);

		if (read_sz == -1) {
			fprintf(stderr, "Can't read from %s: %s\n",
				filename, strerror(errno));
			exit(EXIT_FAILURE);
		} else if (read_sz == 0)
			break;

		fpos += read_sz;

		if ((int)buf_sz - (int)fpos < BUF_SZ) {
			buf_sz += BUF_SZ;
			*buf = yrealloc(*buf, buf_sz);
		}

		bufp = *buf + fpos;
	}

	close(fd);
	return(buf_sz);
}

#define RS_O(a,b) regex_subst(buf, buf_sz, (a), _REG_DEFAULT, (b))
#define RS_G(a,b) regex_subst(buf, buf_sz, (a), _REG_GLOBAL, (b))
#define RS_E(a,b) regex_subst(buf, buf_sz, (a), _REG_EXEC | _REG_GLOBAL, (void*)(b))

static void format_doc(char **buf, size_t *buf_sz)
{
	int i;

	struct subst non_unicode[] = {
		/* utf-8 sequence  , ascii substitution */

		{ "\xE2\x80\x9C\0" , "``" }, /* U+201C */
		{ "\xE2\x80\x9D\0" , "''" }, /* U+201D */
		{ "\xE2\x80\x9E\0" , ",," }, /* U+201E */

		{ "\xE2\x80\x90\0" , "-"  }, /* U+2010 hyphen */
		{ "\xE2\x80\x91\0" , "-"  }, /* U+2011 non-breaking hyphen */
		{ "\xE2\x80\x92\0" , "-"  }, /* U+2012 figure dash */
		{ "\xE2\x80\x93\0" , "-"  }, /* U+2013 en dash */
		{ "\xE2\x80\x94\0" , "--" }, /* U+2014 em dash */
		{ "\xE2\x80\x95\0" , "--" }, /* U+2015 quotation dash */

		{ "\xE2\x80\xA2\0" , "o"  }, /* U+2022 bullet */

		{ "\xE2\x80\xA5\0" , ".." }, /* U+2025 double dot */
		{ "\xE2\x80\xA5\0" , "..."}, /* U+2026 ellipsis */

		{ "\xE2\x86\x90\0" , "<-" }, /* U+2190 left arrow */
		{ "\xE2\x86\x92\0" , "->" }, /* U+2192 right arrow */
		{ "\xE2\x86\x94\0" , "<->"}, /* U+2190 left right arrow */

		{ "\xE2\x82\xAC\0" , "EUR"}, /* U+20AC euro currency symbol */

		{ NULL, NULL },
	};

	/* FIXME: Convert buffer to utf-8 first.  Are there
	   OpenOffice texts which are not utf8-encoded? */

	/* FIXME: only substitute these if output is non-unicode */
	i = 0;
	while(non_unicode[i].utf) {
		RS_G(non_unicode[i].utf, non_unicode[i].ascii);
		i++;
	}

	RS_G("\xEF\x82\xAB\0", "<->"); /* Arrows, symbol font */
	RS_G("\xEF\x82\xAC\0", "<-" );
	RS_G("\xEF\x82\xAE\0", "->" );

	RS_G("&apos;", "'");           /* common entities */
	RS_G("&amp;",  "&");
	RS_G("&quot;", "\"");
	RS_G("&gt;",   ">");
	RS_G("&lt;",   "<");

	/* headline, first level */
	RS_E("<text:h[^>]*outline-level=\"1\"[^>]*>([^<]*)<[^>]*>", &h1);
	RS_E("<text:h[^>]*>([^<]*)<[^>]*>", &h2);  /* other headlines */
	RS_G("<text:p [^>]*>", "\n\n");            /* normal paragraphs */
	RS_G("</text:p>", "\n\n");
	RS_G("<text:tab/>", "  ");                 /* tabs */

/* 	# images */
/* 	s/<draw:frame(.*?)<\/draw:frame>/handle_image($1)/eg; */

	RS_G("<[^>]*>", ""); 	       /* replace all remaining tags */
	RS_G("\n{3,}", "\n\n");        /* remove large vertical spaces */
}

int main(int argc, const char **argv)
{
	struct stat st;
	const char *tmpdir;
	const char *docfile;
	size_t doclen;
	char *docbuf, *outbuf;
	int i = 1;

	setlocale(LC_ALL, "");

	while (argv[i]) {
		if (!strcmp(argv[i], "--raw")) {
			opt_raw = 1;
			i++; continue;
		} else if (!strncmp(argv[i], "--encoding=", 11)) {
			size_t arglen = strlen(argv[i]) - 10;
			opt_encoding = ymalloc(arglen);
			strlcpy(opt_encoding, argv[i] + 11, arglen);
			i++; continue;
		} else if (!strncmp(argv[i], "--width=", 8)) {
			opt_width = atoi(argv[i] + 8);
			if(opt_width < 5)
				usage();
			i++; continue;
		} else if (!strcmp(argv[i], "-")) {
			usage();
		} else {
			if(opt_filename)
				usage();
			opt_filename = argv[i];
			i++; continue;
		}
	}

	if(!opt_filename)
		usage();

	if(!opt_encoding) {
		opt_encoding = nl_langinfo(CODESET);
		if(!opt_encoding) {
			fprintf(stderr, "warning: Could not detect console encoding. Assuming ISO-8859-1\n");
			opt_encoding = "ISO-8859-1";
		}
	}

	if (0 != stat(opt_filename, &st)) {
		fprintf(stderr, "%s: %s\n",
			opt_filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	tmpdir = create_tmpdir();
	docfile = unzip_doc(opt_filename, tmpdir);
	doclen = read_doc(&docbuf, docfile);
	format_doc(&docbuf, &doclen);
	outbuf = conv(docbuf);
	output(outbuf, opt_width);

	/* clean up */
	if(0 != unlink(docfile)) {
		fprintf(stderr, "%s: %s",
			docfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
	yfree((void*)docfile);

	if(0 != rmdir(tmpdir)) {
		fprintf(stderr, "%s: %s",
			tmpdir, strerror(errno));
		exit(EXIT_FAILURE);
	}
	yfree((void*)tmpdir);
	yfree(docbuf);
	yfree(outbuf);

	return EXIT_SUCCESS;
}
