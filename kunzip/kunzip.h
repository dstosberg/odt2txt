#include "../strbuf.h"

/*

ABOUT THIS FILE:

kunzip is a library created by Michael Kohn for dealing with zip files.
This kunzip.h is basically the the documentation on how to make calls
to the library.  If you need help, feel free to email Michael Kohn.

/mike
mike@mikekohn.net
http://www.mikekohn.net/

*/

/*

kunzip_inflate_init - Must be called before any uncompressing is done.
                      Only needs to be called once. 

*/

int kunzip_inflate_init();

/*

kunzip_inflate_free - Must be called when kunzip is no longer needed to
                      free buffers and such.

*/

int kunzip_inflate_free();


STRBUF *kunzip_next_tobuf(char *zip_filename, int offset);

/*

kunzip_get_offset_by_name - Search through a zip archive for a filename
                    that either partially or exactly matches.  If offset
                    is set to -1, the search will start at the start of
                    the file, otherwise the search will start at the next
                    file after the offset.

  Match Flags:
    bit 0: set to 1 if it should be exact filename match
           set to 0 if the archived filename only needs to contain that word
    bit 1: set to 1 if it should be case sensitive
           set to 0 if it should be case insensitive

    in other words:
      match_flags=0: case insensitive, partial match search
      match_flags=1: case insensitive, exact match search
      match_flags=2: case sensitive, partial match search
      match_flags=3: case insensitive, exact match search

Examples:

  offset=kunzip_get_offset_by_name("test.zip","mike.c",0,-1);
  offset=kunzip_get_offset_by_name("test.zip","mike.c",0,offset);

  after the first line above runs, offset will point to the first file
  in test.zip that contains mike.c.  After the second line runs, offset
  will point to the next file in the archive whos filename contains
  mike.c.

*/

int kunzip_get_offset_by_name(char *zip_filename, char *compressed_filename, int match_flags, int skip_offset);

/*

kunzip_get_version - Get the current kunzip library version.

Example:

  char version[128];
  kunzip_get_version(version);
  printf("%s\n",version);

*/

int kunzip_get_version(char *version_string);

