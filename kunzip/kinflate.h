
#include "../strbuf.h"

int inflate(FILE *in, FILE *out, unsigned int *checksum);
int inflate_tobuf(FILE *in, STRBUF *out, unsigned int *checksum);
int inflate_init();
int inflate_free();
unsigned int crc32(unsigned char *buffer, int len, unsigned int crc);



