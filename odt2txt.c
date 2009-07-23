/*
 * odt2txt.c: A simple (and stupid) converter from OpenDocument Text
 *            to plain text.
 *
 * Copyright (c) 2006-2009 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#ifdef NO_ICONV
#  define iconv_t int
#else
#  include <iconv.h>
#  ifdef WIN32
#    include <windows.h>
#  else
#    include <langinfo.h>
#  endif
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
#ifdef HAVE_LIBZIP
#  include <zip.h>
#else
#  include "kunzip/kunzip.h"
#endif

#define VERSION "0.4"

static int opt_raw;
static char *opt_encoding;
static int opt_width = 63;
static const char *opt_filename;
static char *opt_output;

#define SUBST_NONE 0
#define SUBST_SOME 1
#define SUBST_ALL  2

static int opt_subst = SUBST_SOME;

#ifndef ICONV_CHAR
#define ICONV_CHAR char
#endif

#ifdef iconvlist
static void show_iconvlist();
#endif

#define RS_O(a,b) (void)regex_subst(buf, (a), _REG_DEFAULT, (b))
#define RS_G(a,b) (void)regex_subst(buf, (a), _REG_GLOBAL, (b))
#define RS_E(a,b) (void)regex_subst(buf, (a), _REG_EXEC | _REG_GLOBAL, (void*)(b))

static char *guess_encoding(void);
static void write_to_file(STRBUF *outbuf, const char *filename);

struct subst {
	int unicode;
	const char *utf8;
	const char *ascii;
};

static struct subst substs[] = {
       /* number, UTF-8 sequence, ascii substitution */
	{ 0x00A0, "\xC2\xA0",     " "        }, /* no-break space */
	{ 0x00A9, "\xC2\xA9",     "(c)"      }, /* copyright sign */
	{ 0x00AB, "\xC2\xAB",     "&lt;&lt;" }, /* left double angle quote */
	{ 0x00AD, "\xC2\xAD",     "-"        }, /* soft hyphen */
	{ 0x00AE, "\xC2\xAE",     "(r)"      }, /* registered sign */
	{ 0x00BB, "\xC2\xBB",     "&gt;&gt;" }, /* right double angle quote */

	{ 0x00BC, "\xC2\xBC",     "1/4"      }, /* one quarter */
	{ 0x00BD, "\xC2\xBD",     "1/2"      }, /* one half */
	{ 0x00BE, "\xC2\xBE",     "3/4"      }, /* three quarters */

	{ 0x00C4, "\xC3\x84",     "Ae"       }, /* german umlaut A */
	{ 0x00D6, "\xC3\x96",     "Oe"       }, /* german umlaut O */
	{ 0x00DC, "\xC3\x9C",     "Ue"       }, /* german umlaut U */
	{ 0x00DF, "\xC3\x9F",     "ss"       }, /* german sharp s */
	{ 0x00E4, "\xC3\xA4",     "ae"       }, /* german umlaut a */
	{ 0x00F6, "\xC3\xB6",     "oe"       }, /* german umlaut o */
	{ 0x00FC, "\xC3\xBC",     "ue"       }, /* german umlaut u */

	{ 0x2010, "\xE2\x80\x90", "-"        }, /* hyphen */
	{ 0x2011, "\xE2\x80\x91", "-"        }, /* non-breaking hyphen */
	{ 0x2012, "\xE2\x80\x92", "-"        }, /* figure dash */
	{ 0x2013, "\xE2\x80\x93", "-"        }, /* en dash */
	{ 0x2014, "\xE2\x80\x94", "--"       }, /* em dash */
	{ 0x2015, "\xE2\x80\x95", "--"       }, /* quotation dash */

	{ 0x2018, "\xE2\x80\x98", "`"        }, /* single left quotation mark */
	{ 0x2019, "\xE2\x80\x99", "&apos;"   }, /* single right quotation mark */
	{ 0x201A, "\xE2\x80\x9A", ","        }, /* german single right quotation mark */
	{ 0x201B, "\xE2\x80\x9B", "`"        }, /* reversed right quotation mark */
	{ 0x201C, "\xE2\x80\x9C", "``"       }, /* left quotation mark */
	{ 0x201D, "\xE2\x80\x9D", "''"       }, /* right quotation mark */
	{ 0x201E, "\xE2\x80\x9E", ",,"       }, /* german left quotes */

	{ 0x2022, "\xE2\x80\xA2", "o "       }, /* bullet */
	{ 0x2022, "\xE2\x80\xA3", "&lt; "    }, /* triangle bullet */

	{ 0x2025, "\xE2\x80\xA5", ".."       }, /* double dot */
	{ 0x2026, "\xE2\x80\xA6", "..."      }, /* ellipsis */

	{ 0x2030, "\xE2\x80\xB0", "o/oo"     }, /* per mille */
	{ 0x2039, "\xE2\x80\xB9", "&lt;"     }, /* left single angle quote */
	{ 0x203A, "\xE2\x80\xBA", "&gt;"     }, /* right single angle quote */

	{ 0x20AC, "\xE2\x82\xAC", "EUR"      }, /* euro currency symbol */

	{ 0x2190, "\xE2\x86\x90", "&lt;-"    }, /* left arrow */
	{ 0x2192, "\xE2\x86\x92", "-&gt;"    }, /* right arrow */
	{ 0x2194, "\xE2\x86\x94", "&lt;-&gt;"}, /* left right arrow */

	{ 0,      NULL,           NULL },
};

static void usage(void)
{
	printf("odt2txt %s\n"
	       "Converts an OpenDocument or OpenOffice.org XML File to raw text.\n\n"
	       "Syntax:   odt2txt [options] filename\n\n"
	       "Options:  --raw         Print raw XML\n"
#ifdef NO_ICONV
	       "          --encoding=X  Ignored. odt2txt has been built without iconv support.\n"
	       "                        Output will always be encoded in UTF-8\n"
#else
	       "          --encoding=X  Do not try to autodetect the terminal encoding, but\n"
	       "                        convert the document to encoding X unconditionally\n"
#  ifdef iconvlist
	       "                        You can list all supported encodings by specifying\n"
	       "                        --encoding=list\n"
#  endif
	       "                        To find out, which terminal encoding will be used in\n"
	       "                        auto mode, use --encoding=show\n"
#endif
	       "          --width=X     Wrap text lines after X characters. Default: 65.\n"
	       "                        If set to -1 then no lines will be broken\n"
	       "          --output=file Write output to file, instead of STDOUT\n"
	       "          --subst=X     Select which non-ascii characters shall be replaced\n"
	       "                        by ascii look-a-likes:\n"
	       "                           --subst=all   Substitute all characters for which\n"
	       "                                         substitutions are known\n"
	       "                           --subst=some  Substitute all characters which the\n"
	       "                                         output charset does not contain\n"
	       "                                         This is the default\n"
	       "                           --subst=none  Substitute no characters\n"
	       "          --version     Show version and copyright information\n",
	       VERSION);
	exit(EXIT_FAILURE);
}

static void version_info(void)
{
	printf("odt2txt %s\n"
	       "Copyright (c) 2006,2007 Dennis Stosberg <dennis@stosberg.net>\n"
#ifndef HAVE_LIBZIP
	       "Uses the kunzip library, Copyright 2005,2006 by Michael Kohn\n"
#endif
	       "\n"
	       "This program is free software; you can redistribute it and/or\n"
	       "modify it under the terms of the GNU General Public License,\n"
	       "version 2 as published by the Free Software Foundation\n"
	       "\n"
	       "Homepage: http://stosberg.net/odt2txt/\n",
	       VERSION);
	exit(EXIT_SUCCESS);
}

static void yrealloc_buf(char **buf, char **mark, size_t len) {
	ptrdiff_t offset = *mark - *buf;
	*buf = yrealloc(*buf, len);
	*mark = *buf + offset;
}

#ifdef NO_ICONV

static void finish_conv(iconv_t ic)
{
	return;
}

static iconv_t init_conv(const char *input_enc, const char *output_enc)
{
	return 0;
}

static STRBUF *conv(iconv_t ic, STRBUF *buf) {
	STRBUF *output;

	output = strbuf_new();
	strbuf_append_n(output, strbuf_get(buf), strbuf_len(buf));

	return output;
}

static void subst_doc(iconv_t ic, STRBUF *buf) {
	return;
}

static char *guess_encoding(void)
{
	return NULL;
}

#else

static iconv_t init_conv(const char *input_enc, const char *output_enc)
{
	iconv_t ic;
	ic = iconv_open(output_enc, input_enc);
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
	return ic;
}

static void finish_conv(iconv_t ic)
{
	if(iconv_close(ic) == -1) {
		fprintf(stderr, "iconv_close returned: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static STRBUF *conv(iconv_t ic, STRBUF *buf)
{
	/* FIXME: This functionality belongs into strbuf.c */
	ICONV_CHAR *doc;
	char *out, *outbuf;
	size_t inleft, outleft = 0;
	size_t r;
	size_t outlen = 0;
	const size_t alloc_step = 4096;
	STRBUF *output;

	inleft = strbuf_len(buf);
	doc = (ICONV_CHAR*)strbuf_get(buf);
	outlen = alloc_step; outleft = alloc_step;
	outbuf = ymalloc(alloc_step);
	out = outbuf;
	outleft = alloc_step;

	do {
		if (!outleft) {
			outlen += alloc_step; outleft += alloc_step;
			yrealloc_buf(&outbuf, &out, outlen);
		}
		r = iconv(ic, &doc, &inleft, &out, &outleft);
		if (r == (size_t)-1) {
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

				/* advance in source buffer */
				if ((unsigned char)*doc > 0x80)
					skip += utf8_length[(unsigned char)*doc - 0x80];
				doc += skip;
				inleft -= skip;

				/* advance in output buffer */
				*out = '?';
				out++;
				outleft--;

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

	output = strbuf_slurp_n(outbuf, (size_t)(out - outbuf));
	strbuf_setopt(output, STRBUF_NULLOK);
	return output;
}

static void subst_doc(iconv_t ic, STRBUF *buf)
{
	struct subst *s = substs;
	ICONV_CHAR *in;
	size_t inleft;
	const size_t outbuf_sz = 20;
	char *outbuf;
	char *out;
	size_t outleft;
	size_t r;

	if (opt_subst == SUBST_NONE)
		return;

	outbuf = ymalloc(outbuf_sz);
	while (s->unicode) {
		if (opt_subst == SUBST_ALL) {
			RS_G(s->utf8, s->ascii);
		} else {
			out = outbuf;
			outleft = outbuf_sz;
			in = (ICONV_CHAR*)s->utf8;
			inleft = strlen(in);
			r = iconv(ic, &in, &inleft, &out, &outleft);
			if (r == (size_t)-1) {
				if ((errno == EILSEQ) || (errno == EINVAL)) {
					RS_G(s->utf8, s->ascii);
				} else {
					fprintf(stderr,
						"iconv returned an unexpected error: %s\n",
						strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
		}
		s++;
	}
	yfree(outbuf);
}

static char *guess_encoding(void)
{
	char *enc;
	char *tmp;

	enc = ymalloc(20);
#ifdef WIN32
	snprintf(enc, 20, "CP%u", GetACP());
#else
	tmp = nl_langinfo(CODESET);
	strncpy(enc, tmp, 20);
#endif
	if(!enc) {
		fprintf(stderr, "warning: Could not detect console "
			"encoding. Assuming ISO-8859-1\n");
		strncpy(enc, "ISO-8859-1", 20);
	}

	return enc;
}

#endif

static STRBUF *read_from_zip(const char *zipfile, const char *filename)
{
	int r = 0;
	STRBUF *content = NULL;

#ifdef HAVE_LIBZIP
	int zip_error;
	struct zip *zip = NULL;
	struct zip_stat stat;
	struct zip_file *unzipped = NULL;
	char *buf = NULL;

	if ( !(zip = zip_open(zipfile, 0, &zip_error)) ||
	     (r = zip_name_locate(zip, filename, 0)) < 0 ||
	     (zip_stat_index(zip, r, ZIP_FL_UNCHANGED, &stat) < 0) ||
	     !(unzipped = zip_fopen_index(zip, r, ZIP_FL_UNCHANGED)) ) {
		if (unzipped)
			zip_fclose(unzipped);
		if (zip)
			zip_close(zip);
		r = -1;
	}
#else
	r = kunzip_get_offset_by_name((char*)zipfile, (char*)filename, 3, -1);
#endif

	if(-1 == r) {
		fprintf(stderr,
			"Can't read from %s: Is it an OpenDocument Text?\n", zipfile);
		exit(EXIT_FAILURE);
	}

#ifdef HAVE_LIBZIP
	if ( !(buf = ymalloc(stat.size + 1)) ||
	     (zip_fread(unzipped, buf, stat.size) != stat.size) ||
	     !(content = strbuf_slurp_n(buf, stat.size)) ) {
		if (buf)
			yfree(buf);
		content = NULL;
	}
	zip_fclose(unzipped);
	zip_close(zip);
#else
	content = kunzip_next_tobuf((char*)zipfile, r);
#endif

	if (!content) {
		fprintf(stderr,
			"Can't extract %s from %s.  Maybe the file is corrupted?\n",
			filename, zipfile);
		exit(EXIT_FAILURE);
	}

	return content;
}

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
	RS_G("<text:line-break/>", "\n");

	/* images */
	RS_E("<draw:frame[^>]*draw:name=\"([^\"]*)\"[^>]*>", &image);

	RS_G("<[^>]*>", ""); 	 /* replace all remaining tags */
	RS_G("\n +", "\n");      /* remove indentations, e.g. kword */
	RS_G("\n{3,}", "\n\n");  /* remove large vertical spaces */

	RS_G("&apos;", "'");     /* common entities */
	RS_G("&amp;",  "&");
	RS_G("&quot;", "\"");
	RS_G("&gt;",   ">");
	RS_G("&lt;",   "<");

	RS_O("^\n+",  "");       /* blank lines at beginning and end of document */
	RS_O("\n{2,}$",  "\n");
}

int main(int argc, const char **argv)
{
	struct stat st;
	iconv_t ic;
	STRBUF *wbuf;
	STRBUF *docbuf;
	STRBUF *outbuf;
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
			memcpy(opt_encoding, argv[i] + 11, arglen);
			i++; continue;
		} else if (!strncmp(argv[i], "--width=", 8)) {
			opt_width = atoi(argv[i] + 8);
			if(opt_width < 3 && opt_width != -1) {
				fprintf(stderr, "Invalid value for width: %s\n",
					argv[i] + 8);
				exit(EXIT_FAILURE);
			}
			i++; continue;
		} else if (!strcmp(argv[i], "--force")) {
			// ignore this setting
			i++; continue;
		} else if (!strncmp(argv[i], "--output=", 9)) {
			if (*(argv[i] + 9) != '-') {
				size_t arglen = strlen(argv[i]) - 8;
				opt_output = ymalloc(arglen);
				memcpy(opt_output, argv[i] + 9, arglen);
			}
			i++; continue;
		} else if (!strncmp(argv[i], "--subst=", 8)) {
			if (!strcmp(argv[i] + 8, "none"))
				opt_subst = SUBST_NONE;
			else if (!strcmp(argv[i] + 8, "some"))
				opt_subst = SUBST_SOME;
			else if (!strcmp(argv[i] + 8, "all"))
				opt_subst = SUBST_ALL;
			else {
				fprintf(stderr, "Invalid value for --subst: %s\n",
					argv[i] + 8);
				exit(EXIT_FAILURE);
			}
			i++; continue;
		} else if (!strcmp(argv[i], "--help")) {
			usage();
		} else if (!strcmp(argv[i], "--version")
			   || !strcmp(argv[i], "-v")) {
			version_info();
		} else if (!strcmp(argv[i], "-")) {
			usage();
		} else {
			if(opt_filename)
				usage();
			opt_filename = argv[i];
			i++; continue;
		}
	}

	if(opt_encoding && !strcmp("show", opt_encoding)) {
		yfree(opt_encoding);
		opt_encoding = guess_encoding();
		printf("%s\n", opt_encoding);
		yfree(opt_encoding);
		exit(EXIT_SUCCESS);
	}

	if(opt_raw)
		opt_width = -1;

	if(!opt_filename)
		usage();

	if(!opt_encoding) {
		opt_encoding = guess_encoding();
	}

	ic = init_conv("UTF-8", opt_encoding);

	if (0 != stat(opt_filename, &st)) {
		fprintf(stderr, "%s: %s\n",
			opt_filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* read content.xml */
	docbuf = read_from_zip(opt_filename, "content.xml");

	if (!opt_raw) {
		subst_doc(ic, docbuf);
		format_doc(docbuf);
	}

	wbuf = wrap(docbuf, opt_width);

	/* remove all trailing whitespace */
	(void) regex_subst(wbuf, " +\n", _REG_GLOBAL, "\n");

	outbuf = conv(ic, wbuf);

	if (opt_output)
		write_to_file(outbuf, opt_output);
	else
		fwrite(strbuf_get(outbuf), strbuf_len(outbuf), 1, stdout);

	finish_conv(ic);
	strbuf_free(wbuf);
	strbuf_free(docbuf);
	strbuf_free(outbuf);
#ifndef NO_ICONV
	yfree(opt_encoding);
#endif
	if (opt_output)
		yfree(opt_output);

	return EXIT_SUCCESS;
}

static void write_to_file(STRBUF *outbuf, const char *filename)
{
	int fd;
	ssize_t len;

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		fprintf(stderr, "Can't open %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = write(fd, strbuf_get(outbuf), strbuf_len(outbuf));
	if (len == -1) {
		fprintf(stderr, "Can't write to %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(fd);
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
