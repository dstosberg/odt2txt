
int inflate(FILE *in, FILE *out, unsigned int *checksum);
int inflate_init();
int inflate_free();
unsigned int crc32(char *buffer, int len, unsigned int crc);



