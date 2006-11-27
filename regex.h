/*
 * stringops.c: String and regex operations for odt2txt
 *
 * Copyright (c) 2006 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _REG_DEFAULT  0  /* Stop after first match, to be removed */
#define _REG_GLOBAL   1  /* Find all matches of regexp */
#define _REG_EXEC     2  /* subst is a function pointer */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
size_t strlcat(char *dest, const char *src, size_t count);
#endif

/*
  Substitute characters in the string *buf from start to end
  (inclusive) and replace them with *subst. The first character in
  the string has the index 0.

  If *buf is not long enough, it is automaticall realloced and the
  value at buf_sz is set to the new buffer length.

  Returns the change of the length of the string contained at *buf.
  The return value is negative, if the string is shortened, zero if
  the length is unchanged and positive if the length increases.

  If start is bigger than stop, their values are automatically
  swapped.
 */
int buf_subst(char **buf, size_t *buf_sz,
	      size_t start, size_t stop,
	      const char *subst);

/*
  Deletes match(es) of regex from *buf.

  Returns the number of matches that were deleted.
 */
int regex_rm(char **buf, size_t *buf_sz,
	     const char *regex, int regopt);

/*
  Replaces match(es) of regex from *buf with subst.
 */
int regex_subst(char **buf, size_t *buf_sz,
		const char *regex, int regopt,
		const void *subst);

char *underline(char linechar, const char *lenstr);
char *h1(char **buf, regmatch_t matches[], size_t nmatch);
char *h2(char **buf, regmatch_t matches[], size_t nmatch);

void output(char *buf, int width);
