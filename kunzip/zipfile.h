
#define VERSION "Version v1.06 - November 14, 2006"
#define COPYRIGHT "Copyright 2005-2006 - Michael Kohn <mike@mikekohn.net>"

struct zip_local_file_header_t {
	unsigned int signature;
	int version;
	int general_purpose_bit_flag;
	int compression_method;
	int last_mod_file_time;
	int last_mod_file_date;
	unsigned int crc_32;
	int compressed_size;
	int uncompressed_size;
	int file_name_length;
	int extra_field_length;
	char *file_name;
	unsigned char *extra_field;
	int descriptor_length;
};
