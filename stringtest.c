
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "stringops.h"

int main(int argc, char **argv)
{
	char *test1 = "When shall we three meet again?";
	char *test2 = "In thunder, lightning, or in rain?";
	char *test3 =
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do ";

	char *buf;
	size_t buf_sz;

	buf_sz = strlen(test1) + 1;
	buf = ymalloc(buf_sz);
	memcpy(buf, test1, buf_sz);

	/* success: shorter substitution */
	assert( -2 == buf_subst(&buf, &buf_sz, 5, 10, "can"));
	assert((!strcmp(buf, "When can we three meet again?")));

	/* success: same length */
	assert( 0 == buf_subst(&buf, &buf_sz, 5, 8, "CAN"));
	assert((!strcmp(buf, "When CAN we three meet again?")));

	/* success: longer substitution, without realloc */
	assert( 1 == buf_subst(&buf, &buf_sz, 5, 8, "must"));
	assert((!strcmp(buf, "When must we three meet again?")));

	/* success: longer substitution, with realloc */
	assert(14 == buf_subst(&buf, &buf_sz, 4, 9, ", do you think, can"));
	assert((!strcmp(buf, "When, do you think, can we three meet again?")));

	/* success: regexp delete once */
	assert( 1 == regex_rm(&buf, &buf_sz, "in", _REG_DEFAULT));
	assert((!strcmp(buf, "When, do you thk, can we three meet again?")));

	/* success: regexp delete global */
	assert( 2 == regex_rm(&buf, &buf_sz, "ee", _REG_GLOBAL));
	assert((!strcmp(buf, "When, do you thk, can we thr mt again?")));

	yfree(buf);

	buf_sz = strlen(test2) + 1;
	buf = ymalloc(buf_sz);
	memcpy(buf, test2, buf_sz);

	/* delete at beginning */
	assert( 0 == buf_subst(&buf, &buf_sz, 0, 1, "i"));
	assert((!strcmp(buf, "in thunder, lightning, or in rain?")));
	assert(-1 == buf_subst(&buf, &buf_sz, 0, 1, ""));
	assert((!strcmp(buf, "n thunder, lightning, or in rain?")));

	/* delete at end */
	assert( 0 == buf_subst(&buf, &buf_sz, 32, 33, ","));
	assert((!strcmp(buf, "n thunder, lightning, or in rain,")));
	assert(-1 == buf_subst(&buf, &buf_sz, 32, 33, ""));
	assert((!strcmp(buf, "n thunder, lightning, or in rain")));

	/* delete completely */
	assert( 0 - strlen(buf) == buf_subst(&buf, &buf_sz, 0, strlen(buf), ""));
	assert((!strcmp(buf, "")));

	yfree(buf);

	/* test optimization for multiple matches */
	buf = ymalloc(buf_sz);
	memcpy(buf, test2, buf_sz);

	assert( 6 == regex_subst(&buf, &buf_sz, "n", _REG_GLOBAL, ""));
	assert(!strcmp(buf, "I thuder, lightig, or i rai?"));
	assert( 3 == regex_subst(&buf, &buf_sz, "r", _REG_GLOBAL, "R"));
	assert(!strcmp(buf, "I thudeR, lightig, oR i Rai?"));
	assert( 3 == regex_subst(&buf, &buf_sz, "R", _REG_GLOBAL, "R"));
	assert(!strcmp(buf, "I thudeR, lightig, oR i Rai?"));
	assert( 3 == regex_subst(&buf, &buf_sz, "R", _REG_GLOBAL, "RRR"));
	assert(!strcmp(buf, "I thudeRRR, lightig, oRRR i RRRai?"));
	assert( 9 == regex_subst(&buf, &buf_sz, "R", _REG_GLOBAL, "RR"));
	assert(!strcmp(buf, "I thudeRRRRRR, lightig, oRRRRRR i RRRRRRai?"));
	assert( 3 == regex_rm(&buf, &buf_sz, "R+", _REG_GLOBAL));
	assert(!strcmp(buf, "I thude, lightig, o i ai?"));
	assert( 4 == regex_rm(&buf, &buf_sz, "[:alpha:]{0,2}", _REG_GLOBAL));
	assert(!strcmp(buf, " thude, lightig,   ?"));

	yfree(buf);

	/* success: global rm to empty string */
	buf_sz = strlen(test3) + 1;
	buf = ymalloc(buf_sz);
	memcpy(buf, test3, buf_sz);
	assert(50 == regex_rm(&buf, &buf_sz, "do", _REG_GLOBAL));
	assert(50 == regex_rm(&buf, &buf_sz, " ", _REG_GLOBAL));
	assert(!strcmp(buf, ""));
	yfree(buf);

	/* simple patterns */
	buf_sz = 14;
	buf = ymalloc(14);
	memcpy(buf, "abcd35ef8gh6i", 14);
	assert( 1 == regex_rm(&buf, &buf_sz, "[0-9]{2}", _REG_GLOBAL));
	assert( 2 == regex_rm(&buf, &buf_sz, "[0-9]", _REG_GLOBAL));
	assert(!strcmp(buf, "abcdefghi"));

	yfree(buf);

	/* underline 1 */
	buf = underline('=', "Brave new world");
	assert(!strcmp(buf, "Brave new world\n===============\n\n"));
	yfree(buf);

	/* underline 2 */
	buf = underline('-', "");
	assert(!strcmp(buf, ""));
	yfree(buf);

	/* underline 3 */
	buf = underline('B', "AAAAAA");
	assert(!strcmp(buf, "AAAAAA\nBBBBBB\n\n"));
	yfree(buf);

	/* underline 4 */
	buf = underline('-', "");
	assert(!strcmp(buf, ""));
	yfree(buf);

	printf("ALL HAPPY\n");
	return(EXIT_SUCCESS);
}
