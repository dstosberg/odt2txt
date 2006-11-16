#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <iconv.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HAVE_STRLCPY
#include "strlcpy.c"
#endif

static int opt_raw;
static char *opt_encoding;
static int opt_width = 65;
static const char *opt_filename;

void usage(void)
{
	printf("Syntax:   odt2txt [options] filename\n\n"
	       "Converts an OpenDocument Text to raw text.\n\n"
	       "Options:  --raw         Print raw XML\n"
	       "          --encoding=X  Do not try to autodetect the terminal encoding, but\n"
	       "                        convert the document to encoding X unconditionally\n"
	       "          --width=X     Wrap text lines after X characters. Default: 65.\n\n");
	exit(EXIT_FAILURE);
}

const char *tmpdir()
{
	char *dirnam, *tmpdir;
	size_t left = PATH_MAX;
	int r;

	tmpdir = getenv("TMPDIR");
	if (!tmpdir)
		tmpdir = "/home";

	dirnam = malloc(left);
	*dirnam = '\0';
	strlcat(dirnam, tmpdir, left);
	strlcat(dirnam, "/", left);
	strlcpy(dirnam, "odt2txt", left);
	left = strlcpy(dirnam, "/", left);

	if (left > PATH_MAX) {
		exit(1);
	}

	r = mkdir(dirnam, S_IRWXU);
	if (!r) {
		fprintf(stderr, "Cannot create %s: %s", dirnam, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return(dirnam);
}

void realloc_buf(char **buf, char **mark, size_t len) {
	ptrdiff_t offset = *mark - *buf;
	*buf = realloc(*buf, len);
	*mark = *buf + offset;
}

void output(char *docbuf)
{
	iconv_t ic;
	char *doc, *out, *outbuf;
	size_t inleft, outleft = 0;
	size_t conv;
	size_t outlen = 0;
	const size_t alloc_step = 4096;

	char *input_enc = "latin1"; // utf_8
	ic = iconv_open(opt_encoding, input_enc);
	if (ic == (iconv_t)-1) {
		if (errno == EINVAL) {
			fprintf(stderr, "warning: Conversion from %s to %s is not supported.\n", input_enc, opt_encoding);
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
	doc = docbuf;
	outlen = alloc_step; outleft = alloc_step;
	outbuf = malloc(alloc_step);
	out = outbuf;
	outleft = alloc_step;

	do {
		if (!outleft) {
			outlen += alloc_step; outleft += alloc_step;
			realloc_buf(&outbuf, &out, outlen);
		}
		conv = iconv(ic, &doc, &inleft, &out, &outleft);
		if (conv == (size_t)-1) {
			if(errno == E2BIG) {
				continue;
			} else if ((errno == EILSEQ) || (errno == EINVAL)) {
				doc++;
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
		outbuf = realloc(outbuf, outlen + 1);
	}
	*out = '\0';

	if(iconv_close(ic) == -1) {
		fprintf(stderr, "iconv_close returned: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	fputs(outbuf, stdout);
	free(outbuf);
}

void read_odt(char **buf, const char *filename)
{
	char *sample = "asdasd";
	const char *tmp = tmpdir();

	*buf = malloc(50);
	strlcpy(*buf, sample, 50);
}

int main(int argc, const char **argv)
{
	char *docbuf;
	int i = 1;
	setlocale(LC_ALL, "");

	while (argv[i]) {
		if (!strcmp(argv[i], "--raw")) {
			opt_raw = 1;
			i++; continue;
		} else if (!strncmp(argv[i], "--encoding=", 11)) {
			size_t arglen = strlen(argv[i]) - 10;
			opt_encoding = malloc(arglen);
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

	read_odt(&docbuf, opt_filename);

	fprintf(stderr, "debug: raw: %d, encoding: %s, width: %d, file: %s\n",
		opt_raw, opt_encoding, opt_width, opt_filename);

	output(docbuf);

	exit(EXIT_SUCCESS);
}
