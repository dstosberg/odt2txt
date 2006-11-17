
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringops.h"

int main(int argc, char **argv)
{
	char *test1 = "When shall we three meet again?";
	//char *test2 = "In thunder, lightning, or in rain?";
	char *test3 =
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do "
		"do do do do do do do do do do ";

	char *buf;
	size_t buf_sz;

	buf_sz = strlen(test1) + 1;
	buf = malloc(buf_sz);
	memcpy(buf, test1, buf_sz);

	/* fail: start > stop */
	assert(-1 == buf_subst(&buf, &buf_sz, 9, 6, ""));

	/* success: shorter substitution */
	assert( 0 == buf_subst(&buf, &buf_sz, 5, 9, "can"));
	assert((!strcmp(buf, "When can we three meet again?")));

	/* success: same length */
	assert( 0 == buf_subst(&buf, &buf_sz, 5, 7, "CAN"));
	assert((!strcmp(buf, "When CAN we three meet again?")));

	/* success: longer substitution, without realloc */
	assert( 0 == buf_subst(&buf, &buf_sz, 5, 7, "must"));
	assert((!strcmp(buf, "When must we three meet again?")));

	/* success: longer substitution, with realloc */
	assert( 0 == buf_subst(&buf, &buf_sz, 4, 8, ", do you think, can"));
	assert((!strcmp(buf, "When, do you think, can we three meet again?")));

	/* success: regexp delete once */
	assert( 1 == regex_rm(&buf, &buf_sz, "in", REG_ONCE));
	assert((!strcmp(buf, "When, do you thk, can we three meet again?")));

	/* success: regexp delete global */
	assert( 2 == regex_rm(&buf, &buf_sz, "ee", REG_GLOBAL));
	assert((!strcmp(buf, "When, do you thk, can we thr mt again?")));


	/* success: global rm to empty string */
	buf_sz = strlen(test3) + 1;
	buf = malloc(buf_sz);
	memcpy(buf, test3, buf_sz);
	assert(50 == regex_rm(&buf, &buf_sz, "do", REG_GLOBAL));
	assert(50 == regex_rm(&buf, &buf_sz, " ", REG_GLOBAL));
	assert(!strcmp(buf, ""));

	/* simple patterns */
	memcpy(buf, "abcd35ef8gh6i", 14);
	buf_sz = 12;
	assert( 1 == regex_rm(&buf, &buf_sz, "[0-9]{2}", REG_GLOBAL));
	assert( 2 == regex_rm(&buf, &buf_sz, "[0-9]", REG_GLOBAL));
	assert(!strcmp(buf, "abcdefghi"));

	/* underline */
	buf = underline('=', "Brave new world");
	assert(!strcmp(buf, "Brave new world\n==============="));
	buf = underline('-', "");
	assert(!strcmp(buf, ""));
	buf = underline('B', "AAAAAA");
	assert(!strcmp(buf, "AAAAAA\nBBBBBB"));


	printf("%s\n", buf);

	return(EXIT_SUCCESS);
}
