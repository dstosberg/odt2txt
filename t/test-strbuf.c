
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../mem.h"
#include "../strbuf.h"

int main(int argc, char **argv)
{
	STRBUF *buf;
	char *test1 = "When shall we three meet again?";
	char *test2 = "In thunder, lightning, or in rain?";
	char *test3 =
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do ";
	char *c;

	/* trivial */
	buf = strbuf_new();
	strbuf_append(buf, "Hello ");
	strbuf_append(buf, "you!");
	assert(!strcmp(strbuf_get(buf), "Hello you!"));
	strbuf_shrink(buf);
	strbuf_free(buf);

	buf = strbuf_new();
	assert(strlen(test1) == strbuf_append(buf, test1));

	/* success: shorter substitution */
	assert( -2 == strbuf_subst(buf, 5, 10, "can"));
	assert((!strcmp(strbuf_get(buf), "When can we three meet again?")));

	/* success: same length */
	assert( 0 == strbuf_subst(buf, 5, 8, "CAN"));
	assert((!strcmp(strbuf_get(buf), "When CAN we three meet again?")));

	/* success: longer substitution, without realloc */
	assert( 1 == strbuf_subst(buf, 5, 8, "must"));
	assert((!strcmp(strbuf_get(buf), "When must we three meet again?")));

	/* success: longer substitution, with realloc */
	strbuf_shrink(buf);
	assert(14 == strbuf_subst(buf, 4, 9, ", do you think, can"));
	assert((!strcmp(strbuf_get(buf),
			"When, do you think, can we three meet again?")));

	strbuf_free(buf);
	buf = strbuf_new();
	strbuf_append(buf, test2);

	/* delete last char */
	assert( -1 == strbuf_subst(buf, strbuf_len(buf) - 1,
				   strbuf_len(buf), ""));

	/* delete first char */
	assert( -1 == strbuf_subst(buf, 0, 1, ""));
	assert(!strcmp("n thunder, lightning, or in rain", strbuf_get(buf)));

	/* delete completely */
	assert( 0 - strbuf_len(buf)
		== strbuf_subst(buf, 0, strbuf_len(buf), ""));
	assert(!strcmp("", strbuf_get(buf)));

	/* insert a string */
	assert( strlen(test3) == strbuf_subst(buf, 0, 0, test3));
	assert(!strcmp(test3, strbuf_get(buf)));
	strbuf_free(buf);

	/* slurp */
	c = ymalloc(strlen(test2) + 1);
	memcpy(c, test2, strlen(test2) + 1);
	buf = strbuf_create_slurp(c);
	assert(!strcmp(test2, strbuf_get(buf)));
	strbuf_free(buf);

	printf("ALL HAPPY\n");
	return(EXIT_SUCCESS);
}
