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

#ifdef WIN32
#  include <windows.h>
#else
#  include <langinfo.h>
#endif

#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem.h"
#include "regex.h"
#include "strbuf.h"
#include "kunzip/kunzip.h"

#define VERSION "0.1"

static int opt_raw;
static char *opt_encoding;
static int opt_width = 63;
static const char *opt_filename;
static int opt_force;

#ifndef ICONV_CHAR
#define ICONV_CHAR char
#endif

#ifdef iconvlist
static void show_iconvlist();
#endif

struct subst {
	int unicode;
	char *ascii;
};

static int num_substs = -1;
static struct subst substs[] = {
	{ 0x00A0, " "  }, /* no-break space */
	{ 0x00A9, "(c)"}, /* copyright sign */
	{ 0x00AB, "<<" }, /* left double angle quote */
	{ 0x00AD, "-"  }, /* soft hyphen */
	{ 0x00AE, "(r)"}, /* registered sign */
	{ 0x00BB, ">>" }, /* right double angle quote */

	{ 0x00BC, "1/4"}, /* one quarter */
	{ 0x00BD, "1/2"}, /* one half */
	{ 0x00BE, "3/4"}, /* three quarters */

	{ 0x00C4, "Ae" }, /* german umlaut A */
	{ 0x00D6, "Oe" }, /* german umlaut O */
	{ 0x00DC, "Ue" }, /* german umlaut U */
	{ 0x00DF, "ss" }, /* german sharp s */
	{ 0x00E4, "ae" }, /* german umlaut a */
	{ 0x00F6, "oe" }, /* german umlaut o */
	{ 0x00FC, "ue" }, /* german umlaut u */

	{ 0x2010, "-"  }, /* hyphen */
	{ 0x2011, "-"  }, /* non-breaking hyphen */
	{ 0x2012, "-"  }, /* figure dash */
	{ 0x2013, "-"  }, /* en dash */
	{ 0x2014, "--" }, /* em dash */
	{ 0x2015, "--" }, /* quotation dash */

	{ 0x2018, "`"  }, /* single left quotation mark */
	{ 0x2019, "'"  }, /* single right quotation mark */
	{ 0x201A, ","  }, /* german single right quotation mark */
	{ 0x201B, "`"  }, /* reversed right quotation mark */
	{ 0x201C, "``" }, /* left quotation mark */
	{ 0x201D, "''" }, /* right quotation mark */
	{ 0x201E, ",," }, /* german left quotes */

	{ 0x2022, "o " }, /* bullet */
	{ 0x2022, "> " }, /* triangle bullet */

	{ 0x2025, ".." }, /* double dot */
	{ 0x2026, "..."}, /* ellipsis */

	{ 0x2030, "o/oo"},/* per mille */
	{ 0x2039, "<"  }, /* left single angle quote */
	{ 0x203A, ">"  }, /* right single angle quote */

	{ 0x20AC, "EUR"}, /* euro currency symbol */

	{ 0x2190, "<-" }, /* left arrow */
	{ 0x2192, "->" }, /* right arrow */
	{ 0x2194, "<->"}, /* left right arrow */

	{ 0, NULL },
};

static void usage(void)
{
	printf("odt2txt %s\n"
	       "Converts an OpenDocument Text to raw text.\n\n"
	       "Syntax:   odt2txt [options] filename\n\n"
	       "Options:  --raw         Print raw XML\n"
	       "          --encoding=X  Do not try to autodetect the terminal encoding, but\n"
	       "                        convert the document to encoding X unconditionally\n"
#ifdef iconvlist
	       "                        You can list all supported encodings by specifying\n"
	       "                        --encoding=list\n"
#endif
	       "          --width=X     Wrap text lines after X characters. Default: 65.\n"
	       "                        If set to -1 then no lines will be broken\n"
	       "          --force       Do not stop if the mimetype if unknown.\n",
	       VERSION);
	exit(EXIT_FAILURE);
}

static void init_substs() {
	struct subst *s = substs;

	while(s->unicode) {
		s++;
		num_substs++;
	}
}

static const char *get_subst(int uc)
{
	struct subst *start = substs;
	struct subst *end   = substs + num_substs;
	struct subst *cur;

	while (start + 1 < end) {
		cur = start + ((end - start) / 2);
		if (uc > cur->unicode)
			start = cur;
		else
			end = cur;
	}

	if (uc == end->unicode)
		return end->ascii;

	if (uc == start->unicode)
		return start->ascii;

	return NULL;
}

static int utf8_to_uc(const char *utf8)
{
	const unsigned char *in = utf8;

	if (!(*in & 0x80)) {               /* 0xxxxxxx */
		return *in;
	} else if ((*in & 0xE0) == 0xC0) { /* 110xxxxx 10xxxxxx */
		return ((*in & 0x1F) << 6) + (*(in+1) & 0x3F);
	} else if ((*in & 0xF0) == 0xE0) { /* 1110xxxx 10xxxxxx 10xxxxxx */
		return ((*in & 0x0F) << 12) + ((*(in+1) & 0x3F) << 6) + (*(in+2) & 0x3F);
	} else if ((*in & 0xF8) == 0xF0) { /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
		return ((*in & 0x07) << 18) + ((*(in+1) & 0x3F) << 12)
			+ ((*(in+2) & 0x3F) << 6) + ((*(in+3) & 0x3F));
	}
	/* rest is reserved */
	return -1;
}

static void yrealloc_buf(char **buf, char **mark, size_t len) {
	ptrdiff_t offset = *mark - *buf;
	*buf = yrealloc(*buf, len);
	*mark = *buf + offset;
}

static STRBUF *conv(STRBUF *buf)
{
	/* FIXME: This functionality belongs into strbuf.c */
	iconv_t ic;
	ICONV_CHAR *doc;
	char *out, *outbuf;
	size_t inleft, outleft = 0;
	size_t conv;
	size_t outlen = 0;
	const size_t alloc_step = 4096;
	STRBUF *output;

	char *input_enc = "utf-8";
	ic = iconv_open(opt_encoding, input_enc);
	if (ic == (iconv_t)-1) {
		if (errno == EINVAL) {
			fprintf(stderr, "warning: Conversion from %s to %s is not supported.\n",
				input_enc, opt_encoding);
			ic = iconv_open("us-ascii", input_enc);
			if (ic == (iconv_t)-1) {
				exit(EXIT_FAILURE);
			}
			fprintf(stderr, "warning: Using us-ascii as fall-back.\n");
		} else {
			fprintf(stderr, "iconv_open returned: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	inleft = strbuf_len(buf);
	doc = (ICONV_CHAR*)strbuf_get(buf);
	outlen = alloc_step; outleft = alloc_step;
	outbuf = ymalloc(alloc_step);
	out = outbuf;
	outleft = alloc_step;

	init_substs();

	do {
		if (!outleft) {
			outlen += alloc_step; outleft += alloc_step;
			yrealloc_buf(&outbuf, &out, outlen);
		}
		conv = iconv(ic, &doc, &inleft, &out, &outleft);
		if (conv == (size_t)-1) {
			if(errno == E2BIG) {
				outlen += alloc_step; outleft += alloc_step;
				if (outlen > (strbuf_len(buf) << 3)) {
					fprintf(stderr, "Buffer grew to much. "
						"Corrupted document?\n");
					exit(EXIT_FAILURE);
				}
				yrealloc_buf(&outbuf, &out, outlen);
				continue;
			} else if ((errno == EILSEQ) || (errno == EINVAL)) {
				char skip = 1;
				const char *subst = get_subst(utf8_to_uc(doc));

				/* do we have a substitution? */
				if (!subst)
					subst = "?";
				while(*subst) {
					*out = *subst;
					out++;
					subst++;
					outleft--;
				}

				/* advance in source buffer */
				if ((unsigned char)*doc > 0x80)
					skip += utf8_length[(unsigned char)*doc - 0x80];
				doc += skip;
				inleft -= skip;

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

	output = strbuf_slurp_n(outbuf, (size_t)(out - outbuf));
	strbuf_setopt(output, STRBUF_NULLOK);
	return output;
}

static STRBUF *read_from_zip(const char *zipfile, const char *filename)
{
	int r;
	STRBUF *content;

	r = kunzip_get_offset_by_name((char*)zipfile, (char*)filename, 3, -1);

	if(-1 == r) {
		fprintf(stderr,
			"Can't read from %s: Is it an OpenDocument Text?\n", zipfile);
		exit(EXIT_FAILURE);
	}

	content = kunzip_next_tobuf((char*)zipfile, r);

	if (!content) {
		fprintf(stderr,
			"Can't extract %s from %s.  Maybe the file is corrupted?\n",
			filename, zipfile);
		exit(EXIT_FAILURE);
	}

	return content;
}

#define RS_O(a,b) (void)regex_subst(buf, (a), _REG_DEFAULT, (b))
#define RS_G(a,b) (void)regex_subst(buf, (a), _REG_GLOBAL, (b))
#define RS_E(a,b) (void)regex_subst(buf, (a), _REG_EXEC | _REG_GLOBAL, (void*)(b))

static void format_doc(STRBUF *buf)
{
	/* FIXME: Convert buffer to utf-8 first.  Are there
	   OpenOffice texts which are not utf8-encoded? */

	/* headline, first level */
	RS_E("<text:h[^>]*outline-level=\"1\"[^>]*>([^<]*)<[^>]*>", &h1);
	RS_E("<text:h[^>]*>([^<]*)<[^>]*>", &h2);  /* other headlines */
	RS_G("<text:p [^>]*>", "\n\n");            /* normal paragraphs */
	RS_G("</text:p>", "\n\n");
	RS_G("<text:tab/>", "  ");                 /* tabs */

/* 	# images */
/* 	s/<draw:frame(.*?)<\/draw:frame>/handle_image($1)/eg; */

	RS_G("<[^>]*>", ""); 	       /* replace all remaining tags */
	RS_G("\n +", "\n");            /* remove indentations, e.g. kword */
	RS_G("\n{3,}", "\n\n");        /* remove large vertical spaces */

	RS_G("&apos;", "'");           /* common entities */
	RS_G("&amp;",  "&");
	RS_G("&quot;", "\"");
	RS_G("&gt;",   ">");
	RS_G("&lt;",   "<");
}

int main(int argc, const char **argv)
{
	struct stat st;
	int free_opt_enc = 0;
	STRBUF *wbuf;
	STRBUF *docbuf;
	STRBUF *outbuf;
	STRBUF *mimetype;
	int i = 1;

	(void)setlocale(LC_ALL, "");

	while (argv[i]) {
		if (!strcmp(argv[i], "--raw")) {
			opt_raw = 1;
			i++; continue;
		} else if (!strncmp(argv[i], "--encoding=", 11)) {
			size_t arglen = strlen(argv[i]) - 10;
#ifdef iconvlist
			if (!strcmp(argv[i] + 11, "list")) {
				show_iconvlist();
			}
#endif
			opt_encoding = ymalloc(arglen);
			free_opt_enc = 1;
			memcpy(opt_encoding, argv[i] + 11, arglen);
			i++; continue;
		} else if (!strncmp(argv[i], "--width=", 8)) {
			opt_width = atoi(argv[i] + 8);
			if(opt_width < -1)
				usage();
			i++; continue;
		} else if (!strcmp(argv[i], "--force")) {
			opt_force = 1;
			i++; continue;
		} else if (!strcmp(argv[i], "--help")) {
			usage();
		} else if (!strcmp(argv[i], "-")) {
			usage();
		} else {
			if(opt_filename)
				usage();
			opt_filename = argv[i];
			i++; continue;
		}
	}

	if(opt_raw)
		opt_width = -1;

	if(!opt_filename)
		usage();

	if(!opt_encoding) {
#ifdef WIN32
		opt_encoding = ymalloc(20);
		free_opt_enc = 1;
		snprintf(opt_encoding, 20, "CP%u", GetACP());
#else
		opt_encoding = nl_langinfo(CODESET);
#endif
		if(!opt_encoding) {
			fprintf(stderr, "warning: Could not detect console "
				"encoding. Assuming ISO-8859-1\n");
			opt_encoding = "ISO-8859-1";
		}
	}

	if (0 != stat(opt_filename, &st)) {
		fprintf(stderr, "%s: %s\n",
			opt_filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	(void)kunzip_inflate_init();

	/* check mimetype */
	mimetype = read_from_zip(opt_filename, "mimetype");

	if (0 == strcmp("application/vnd.oasis.opendocument.text",
			strbuf_get(mimetype))
	    && 0 == strcmp("application/vnd.sun.xml.writer",
			   strbuf_get(mimetype))
	    && !opt_force) {
		fprintf(stderr, "Document has unknown mimetype: -%s-\n",
			strbuf_get(mimetype));
		fprintf(stderr, "Won't continue without --force.\n");
		strbuf_free(mimetype);
		exit(EXIT_FAILURE);
	}
	strbuf_free(mimetype);

	/* read content.xml */
	docbuf = read_from_zip(opt_filename, "content.xml");
	(void)kunzip_inflate_free();

	if (!opt_raw)
		format_doc(docbuf);

	wbuf = wrap(docbuf, opt_width);
	outbuf = conv(wbuf);
	fwrite(strbuf_get(outbuf), strbuf_len(outbuf), 1, stdout);

	strbuf_free(wbuf);
	strbuf_free(docbuf);
	strbuf_free(outbuf);

	if (free_opt_enc)
		yfree(opt_encoding);

	return EXIT_SUCCESS;
}

#ifdef iconvlist
static int print_one (unsigned int namescount, const char * const * names,
                      void *data)
{
	int i;

	for (i = 0; i < namescount; i++) {
		if (i > 0)
			putc(' ',stdout);
		fputs(names[i],stdout);
	}
	putc('\n',stdout);
	return 0;
}

static void show_iconvlist() {
	iconvlist(print_one, NULL);
	exit(EXIT_SUCCESS);
}
#endif
