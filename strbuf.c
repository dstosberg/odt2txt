/*
 * strbuf.c: A simple string buffer
 *
 * Copyright (c) 2006,2007 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include "strbuf.h"

static const size_t strbuf_start_sz = 128;
static const size_t strbuf_grow_sz = 128;

static void strbuf_grow(STRBUF *buf); /* enlarge a buffer by strbuf_grow_sz */

#ifdef STRBUF_CHECK
static void die(const char *format, ...) {
	va_list argp;
	va_start(argp, format);
	vfprintf(stderr, format, argp);
	va_end(argp);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

static void strbuf_check(STRBUF *buf)
{
	if (!buf)
		die("buf is null");

	if (!buf->data)
		die("buf->data is null");

	if (!(buf->opt & STRBUF_NULLOK) && strlen(buf->data) != buf->len)
		die("length mismatch. strlen says %u, len says %u",
		    (unsigned int)strlen(buf->data), buf->len);

	if (buf->len + 1 > buf->buf_sz)
		die("overlap");
}
#else
#define strbuf_check(a)
#endif

STRBUF *strbuf_new(void)
{
	STRBUF *buf = ymalloc(sizeof(STRBUF));

	buf->buf_sz = strbuf_start_sz;
	buf->data = ymalloc(strbuf_start_sz);

	buf->len = 0;
	buf->data[0] = '\0';

	buf->opt = 0;

	strbuf_check(buf);
	return buf;
}

void strbuf_free(STRBUF *buf)
{
	strbuf_check(buf);

	yfree(buf->data);
	yfree(buf);
}

void strbuf_shrink(STRBUF *buf)
{
	strbuf_check(buf);

	buf->buf_sz = buf->len + 1;
	buf->data = yrealloc(buf->data, buf->buf_sz);

	strbuf_check(buf);
}

size_t strbuf_append_n(STRBUF *buf, const char *str, size_t n)
{
	strbuf_check(buf);

	if (n == 0)
		return buf->len;

	while (buf->len + n + 1 > buf->buf_sz)
		strbuf_grow(buf);

	memcpy(buf->data + buf->len, str, n);
	buf->len += n;
	*(buf->data + buf->len) = 0;

	strbuf_check(buf);
	return buf->len;
}

size_t strbuf_append(STRBUF *buf, const char *str)
{
	return strbuf_append_n(buf, str, strlen(str));
}

const char *strbuf_get(STRBUF *buf)
{
	strbuf_check(buf);
	return buf->data;
}

size_t strbuf_len(STRBUF *buf)
{
	strbuf_check(buf);
	return buf->len;
}

int strbuf_subst(STRBUF *buf,
		 size_t start, size_t stop,
		 const char *subst)
{
	size_t len;
	size_t subst_len;
	int    diff;

	strbuf_check(buf);

	if (start > stop) {
		size_t tmp = start;
		start = stop;
		stop = tmp;
	}

	len = stop - start;
	subst_len = strlen(subst);
	diff = subst_len - len;

	if (0 > diff) {
		memcpy(buf->data + start, subst, subst_len);
		memmove(buf->data + start + subst_len, buf->data + stop,
			buf->len - stop + 1);

	} else if (0 == diff) {
		memcpy(buf->data + start, subst, subst_len);

	} else { /* 0 < diff */
		while (buf->len + diff + 1 > buf->buf_sz)
			strbuf_grow(buf);

		memmove(buf->data + start + subst_len, buf->data + stop,
			buf->len - stop + 1);
		memcpy(buf->data + start, subst, subst_len);
	}

	buf->len += diff;

	strbuf_check(buf);
	return diff;
}

static void strbuf_grow(STRBUF *buf)
{
	strbuf_check(buf);

	buf->buf_sz += strbuf_grow_sz;
	buf->data = yrealloc(buf->data, buf->buf_sz);

	strbuf_check(buf);
}

STRBUF *strbuf_slurp(char *str)
{
	return strbuf_slurp_n(str, strlen(str));
}

STRBUF *strbuf_slurp_n(char *str, size_t len)
{
	STRBUF *buf = ymalloc(sizeof(STRBUF));
	buf->len = len;
	buf->buf_sz = len + 1;
	buf->data = yrealloc(str, buf->buf_sz);
	*(buf->data + len) = '\0';

	buf->opt = 0;

	return buf;
}

char *strbuf_spit(STRBUF *buf)
{
	char *data;

	strbuf_check(buf);

	strbuf_shrink(buf);
	data = buf->data;
	yfree(buf);

	return data;
}

void strbuf_setopt(STRBUF *buf, enum strbuf_opt opt)
{
	buf->opt |= opt;
}
