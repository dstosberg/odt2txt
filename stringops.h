
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
size_t strlcat(char *dest, const char *src, size_t count);
#endif

/*
  Substitute characters in the string *buf from start to end
  (inclusive) and replace them with *subst.

  If *buf is not long enough, it is automaticall realloced and the
  value at buf_sz is set to the new buffer length.

  Return value: 0 on success, -1 on failure
 */
int buf_subst(char **buf, size_t *buf_sz, size_t start, size_t stop, const char *subst);


enum regex_opt {
	REG_GLOBAL, /* Find all matches of regexp */
	REG_ONCE,   /* Stop after first match */
};

/*
  Deletes match(es) of regex from *buf.
 */
int regex_rm(char **buf, size_t *buf_sz,
	     const char *regex, enum regex_opt regopt);

/*
  Replaces match(es) of regex from *buf with subst.
 */
int regex_subst(char **buf, size_t *buf_sz,
		const char *regex, enum regex_opt regopt,
		const char *subst);


char *underline(char linechar, const char *lenstr);


