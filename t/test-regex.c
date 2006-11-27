
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../mem.h"
#include "../strbuf.h"
#include "../regex.h"

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

	/* test optimization for multiple matches */
	buf = strbuf_new();
	strbuf_append(buf, test2);

	assert( 6 == regex_subst(buf, "n", _REG_GLOBAL, ""));
	assert(!strcmp(strbuf_get(buf), "I thuder, lightig, or i rai?"));
	assert( 3 == regex_subst(buf, "r", _REG_GLOBAL, "R"));
	assert(!strcmp(strbuf_get(buf), "I thudeR, lightig, oR i Rai?"));
	assert( 3 == regex_subst(buf, "R", _REG_GLOBAL, "R"));
	assert(!strcmp(strbuf_get(buf), "I thudeR, lightig, oR i Rai?"));
	assert( 3 == regex_subst(buf, "R", _REG_GLOBAL, "RRR"));
	assert(!strcmp(strbuf_get(buf), "I thudeRRR, lightig, oRRR i RRRai?"));
	assert( 9 == regex_subst(buf, "R", _REG_GLOBAL, "RR"));
	assert(!strcmp(strbuf_get(buf), "I thudeRRRRRR, lightig, oRRRRRR i RRRRRRai?"));
	assert( 3 == regex_rm(buf, "R+", _REG_GLOBAL));
	assert(!strcmp(strbuf_get(buf), "I thude, lightig, o i ai?"));
	strbuf_free(buf);

	/* global rm to empty string */
	buf = strbuf_new();
	strbuf_append(buf, test3);
	assert(50 == regex_rm(buf, "do", _REG_GLOBAL));
	assert(50 == regex_rm(buf, " ", _REG_GLOBAL));
	assert(!strcmp(strbuf_get(buf), ""));
	strbuf_free(buf);

	/* simple patterns */
	buf = strbuf_new();
	strbuf_append(buf, "abcd35ef8gh6i");
	assert( 1 == regex_rm(buf, "[0-9]{2}", _REG_GLOBAL));
	assert( 2 == regex_rm(buf, "[0-9]", _REG_GLOBAL));
	assert(!strcmp(strbuf_get(buf), "abcdefghi"));
	strbuf_free(buf);

	/* underline 1 */
	c = underline('=', "Brave new world");
	assert(!strcmp(c, "Brave new world\n===============\n\n"));
	yfree(c);

	/* underline 2 */
	c = underline('-', "");
	assert(!strcmp(c, ""));
	yfree(c);

	/* underline 3 */
	c = underline('B', "AAAAAA");
	assert(!strcmp(c, "AAAAAA\nBBBBBB\n\n"));
	yfree(c);

	/* underline 4 */
	c = underline('-', "");
	assert(!strcmp(c, ""));
	yfree(c);


	printf("ALL HAPPY\n");
	return(EXIT_SUCCESS);
}
