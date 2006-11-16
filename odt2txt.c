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

#include "kunzip/kunzip.h"

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

const char *create_tmpdir()
{
	char *dirnam;
	char *tmpdir;
	char *pid;
	size_t pidlen;
	size_t left = PATH_MAX;
	int r;

	pid = malloc(20);
	pidlen = snprintf(pid, 20,  "%d", (int)getpid());
	if (pidlen >= 20 || pidlen <= 0) {
		fprintf(stderr, "Couldn't get pid\n");
		exit(EXIT_FAILURE);
	}

	tmpdir = getenv("TMPDIR");
	if (!tmpdir)
		tmpdir = "/tmp";

	dirnam = malloc(left);
	*dirnam = '\0';
	strlcat(dirnam, tmpdir, left);
	strlcat(dirnam, "/", left);
	strlcat(dirnam, "odt2txt-", left);
	strlcat(dirnam, pid, left);
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

/*
void read_odt(char **buf, const char *filename)
{
	int offset;
	const char *tmpdir = create_tmpdir();
	const char *sample = " balsdcasdc\n";
	const char *content_file = malloc(PATH_MAX);


	kunzip_inflate_init();
	offset = kunzip_get_offset_by_name((char*)filename, "content.xml", 3, -1);

	if(!offset) {
		fprintf(stderr, "Can't open %s: Is it an OpenDocument Text?\n", filename);
		exit(EXIT_FAILURE);
	}

	kunzip_next((char*)filename, (char*)tmpdir, offset);

	

	*buf = malloc(50);
	snprintf(*buf, 50, "%s", filename);
	strlcat(*buf, sample, 50);

	kunzip_inflate_free();
}
*/

const char *unzip_doc(const char *filename, const char *tmpdir)
{
	int r;
	char *content_file;
	struct stat st;

	kunzip_inflate_init();
	r = kunzip_get_offset_by_name((char*)filename, "content.xml", 3, -1);

	if(!r) {
		fprintf(stderr, "Can't open %s: Is it an OpenDocument Text?\n", filename);
		exit(EXIT_FAILURE);
	}

	r = kunzip_next((char*)filename, (char*)tmpdir, r);
	kunzip_inflate_free();

	content_file = malloc(PATH_MAX);
	*content_file = '\0';
	strlcpy(content_file, tmpdir, PATH_MAX);
	r = strlcat(content_file, "content.xml", PATH_MAX);

	if (r >= PATH_MAX) {
		fprintf(stderr, "unzip_doc: name too long\n");
		exit(EXIT_FAILURE);
	}

	r = stat(content_file, &st);
	if (r) {
		fprintf(stderr, "Unzipping file failed. %s does not exist: %s\n",
			content_file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return(content_file);
}

int main(int argc, const char **argv)
{
	const char *tmpdir;
	const char *docfile;
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

	tmpdir = create_tmpdir();
	docfile = unzip_doc(opt_filename, tmpdir);

	//read_odt(&docbuf, opt_filename);

	fprintf(stderr, "debug: raw: %d, encoding: %s, width: %d, file: %s, docfile: %s\n",
		opt_raw, opt_encoding, opt_width, opt_filename, docfile);

	//output(docbuf);

	exit(EXIT_SUCCESS);
}
