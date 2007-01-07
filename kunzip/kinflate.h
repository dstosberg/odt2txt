
#include "../strbuf.h"

int inflate(FILE *in, FILE *out, unsigned int *checksum);
int inflate_tobuf(FILE *in, STRBUF *out, unsigned int *checksum);
int inflate_init(void);
int inflate_free(void);
unsigned int crc32(unsigned char *buffer, int len, unsigned int crc);



