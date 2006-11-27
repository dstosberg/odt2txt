/*
 * stringops.c: String and regex operations for odt2txt
 *
 * Copyright (c) 2006 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include "mem.h"
#include "regex.h"

#define BUF_SZ 4096

static char *headline(char line, char **buf, regmatch_t matches[], size_t nmatch);
static size_t charlen_utf8(const char *s);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size)
{
        size_t ret = strlen(src);

        if (size) {
                size_t len = (ret >= size) ? size - 1 : ret;
                memcpy(dest, src, len);
                dest[len] = '\0';
        }
        return ret;
}

size_t strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;

	if (dsize >= count) {
		fprintf(stderr, "strlcat: destination string not terminated?");
		exit(EXIT_FAILURE);
	}

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count-1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}
#endif

int buf_subst(char **buf, size_t *buf_sz,
	      size_t start, size_t stop,
	      const char *subst)
{
	size_t len;
	size_t subst_len;
	int    diff;

	if (start > stop) {
		size_t tmp = start;
		start = stop;
		stop = tmp;
	}

	len = stop - start;
	subst_len = strlen(subst);
	diff = subst_len - len;

	if (0 > diff) {
		memcpy(*buf + start, subst, subst_len);
		memmove(*buf + start + subst_len, *buf + stop, *buf_sz - stop);

	} else if (0 == diff) {
		memcpy(*buf + start, subst, subst_len);

	} else { /* 0 < diff */
		int l = strlen(*buf);
		while (l + diff + 1 > *buf_sz) {

			/* safety measure: do not swap to death if
			   the final \0 is missing. */
			if (l >= *buf_sz) {
				fprintf(stderr, "missing string terminator\n");
				exit(EXIT_FAILURE);
			}

			*buf_sz += BUF_SZ;
			*buf = yrealloc(*buf, *buf_sz);
		}
		memmove(*buf + start + subst_len, *buf + stop,
			(*buf_sz - stop) - diff);
		memcpy(*buf + start, subst, subst_len);
	}

	return diff;
}

void print_regexp_err(int reg_errno, const regex_t *rx)
{
	char *buf = ymalloc(BUF_SZ);

	regerror(reg_errno, rx, buf, BUF_SZ);
	fprintf(stderr, "%s\n", buf);

	yfree(buf);
}

int regex_subst(char **buf, size_t *buf_sz,
		const char *regex, int regopt,
		const void *subst)
{
	int r;
	char *bufp;
	size_t off = 0;
	const int i = 0;
	int match_count = 0;

	regex_t rx;
	const size_t nmatches = 10;
	regmatch_t matches[nmatches];

	r = regcomp(&rx, regex, REG_EXTENDED);
	if (r) {
		print_regexp_err(r, &rx);
		exit(EXIT_FAILURE);
	}

	do {
		bufp = *buf + off;
		if(0 != regexec(&rx, bufp, nmatches, matches, 0))
			break;

		if (matches[i].rm_so != -1) {
			char *s;
			int subst_len;

			if (regopt & _REG_EXEC) {
				s = (*(char *(*)
				       (char **buf, regmatch_t matches[],
					size_t nmatch))subst)
					(buf, matches, nmatches);
			} else
				s = (char*)subst;

			subst_len = buf_subst(buf, buf_sz,
					      matches[i].rm_so + off,
					      matches[i].rm_eo + off,
					      s);
			match_count++;

			if (regopt & _REG_EXEC)
				yfree(s);

			off += matches[i].rm_so;
			if (subst_len >= 0)
				off += subst_len + 1;
		}
	} while (regopt & _REG_GLOBAL);

	regfree(&rx);
	return match_count;
}

int regex_rm(char **buf, size_t *buf_sz,
	     const char *regex, int regopt)
{
	return regex_subst(buf, buf_sz, regex, regopt, "");
}

char *underline(char linechar, const char *lenstr)
{
	int i;
	char *line;
	size_t len = strlen(lenstr);
	size_t charlen = charlen_utf8(lenstr);
	size_t linelen = len + charlen + 2;

	if (!len) {
		line = ymalloc(1);
		line[0] = '\0';
		return line;
	}

	line = ymalloc(linelen + 3);
	strlcpy(line, lenstr, linelen);
	strlcat(line, "\n", linelen);
	for (i = len + 1; i < linelen - 1; i++) {
		line[i] = linechar;
	}
	line[linelen - 1] = '\n';
	line[linelen    ] = '\n';
	line[linelen + 1] = '\0';

	return line;
}

static char *headline(char line, char **buf, regmatch_t matches[], size_t nmatch)
{
	const int i = 1;
	char *result;
	size_t len;
	char *match;

	len = matches[i].rm_eo - matches[i].rm_so;
	match = ymalloc(len + 1);

	memcpy(match, *buf + matches[i].rm_so, len);
	match[len] = '\0' ;

	result = underline(line, match);

	yfree(match);
	return result;
}

char *h1(char **buf, regmatch_t matches[], size_t nmatch)
{
	return headline('=', buf, matches, nmatch);
}

char *h2(char **buf, regmatch_t matches[], size_t nmatch)
{
	return headline('-', buf, matches, nmatch);
}

static size_t charlen_utf8(const char *s)
{
	size_t count = 0;
	unsigned char *t = (unsigned char*) s;
	while (*t != '\0') {
		if (*t > 0x80)
			t++;
		if (*t > 0xDF)
			t++;
		if (*t > 0xF0)
			t++;
		count++;
		t++;
	}
	return count;
}

void output(char *buf, int width)
{
	/* FIXME: This function should take multibyte utf8-encoded
	   characters into account for the length calculation. */

	const char *lf = "\n  ";
	const size_t lflen = strlen(lf);
	const char *bufp = buf;
	const char *last = buf;
	const char *lastspace = 0;
	int linelen = 0;

	fwrite(lf, lflen, 1, stdout);
	while (*bufp) {
		if (*bufp == ' ')
			lastspace = bufp;
		else if (*bufp == '\n') {
			while(*last == ' ')
				last++;

			fwrite(last, (size_t)(bufp - last), 1, stdout);
			fwrite(lf, lflen, 1, stdout);
 			last = bufp + 1;
			linelen = 0;
		}

		if (linelen >= width) {
			while(*last == ' ')
				last++;

			fwrite(last, (size_t)(lastspace - last), 1, stdout);
			fwrite(lf, lflen, 1, stdout);
			last = lastspace;
			linelen = 0;
		}

		bufp++;
		linelen++;
	}
	fputs("\n", stdout);
}

