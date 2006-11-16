

int write_int(FILE *out, int n);
int write_word(FILE *out, int n);

int read_int(FILE *in);
int read_word(FILE *in);

int read_chars(FILE *in, char *s, int count);
int write_chars(FILE *out, char *s);

int write_int_b(FILE *out, int n);
int write_word_b(FILE *out, int n);

int read_int_b(FILE *in);
int read_word_b(FILE *in);

int read_buffer(FILE *in, unsigned char *buffer, int len);
int write_buffer(FILE *in, unsigned char *buffer, int len);

