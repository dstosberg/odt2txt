
#include "stringops.h"

#define BUF_SZ 4096

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

int buf_subst(char **buf, size_t *buf_sz, size_t start, size_t stop, const char *subst)
{
	int len;
	size_t subst_len;

	if (start > stop)
		return -1;

	len = (stop - start) + 1;
	subst_len = strlen(subst);

	if (subst_len <= len) {
		memcpy(*buf + start, subst, subst_len);
		memmove(*buf + start + subst_len, *buf + stop + 1, (*buf_sz - stop) + 1);
	} else {
		while (strlen(*buf) + 1 + (subst_len - len) > *buf_sz) {
			*buf_sz += BUF_SZ;
			*buf = realloc(*buf, *buf_sz);
		}
		memmove(*buf + start + subst_len, *buf + stop + 1, (*buf_sz - stop) + 1);
		memcpy(*buf + start, subst, subst_len);
	}

	return 0;
}

void print_regexp_err(int reg_errno, const regex_t *rx)
{
	char *buf = malloc(BUF_SZ);

	regerror(reg_errno, rx, buf, BUF_SZ);
	fprintf(stderr, "%s\n", buf);

	free(buf);
}

int regex_subst(char **buf, size_t *buf_sz,
		const char *regex, enum regex_opt regopt,
		const char *subst)
{
	int r;
	int i = 0;
	int num_matches = 0;

	regex_t rx;
	regmatch_t matches[1];

	r = regcomp(&rx, regex, REG_EXTENDED);
	if (r) {
		print_regexp_err(r, &rx);
		exit(EXIT_FAILURE);
	}

	do {
		if(0 != regexec(&rx, *buf, 1, matches, 0))
			break;

		if (matches[i].rm_so != -1) {
			buf_subst(buf, buf_sz,
			      matches[i].rm_so, matches[i].rm_eo - 1,
			      subst);
			num_matches++;
		}
	} while (regopt == REG_GLOBAL);

	regfree(&rx);
	return num_matches;
}

int regex_rm(char **buf, size_t *buf_sz,
	     const char *regex, enum regex_opt regopt)
{
	return regex_subst(buf, buf_sz, regex, regopt, "");
}

char *underline(char linechar, const char *lenstr)
{
	int i;
	char *line;
	size_t len = strlen(lenstr);
	size_t linelen = 2 * len + 2;

	if (!len) {
		line = malloc(1);
		line[0] = '\0';
		return line;
	}

	line = malloc(linelen);
	strlcpy(line, lenstr, linelen);
	strlcat(line, "\n", linelen);
	for (i = len + 1; i < linelen - 1; i++) {
		line[i] = linechar;
	}
	line[linelen - 1] = '\0';

	return line;
}

