
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

/*

kunzip_all - Unzips all files in a zip archive into the base dir.

Example: kunzip_all("test.zip","/tmp");

Will unzip all files from test.zip into the /tmp dir

*/

int kunzip_all(char *zip_filename, char *base_dir);

/*

kunzip next - Unzips the file in a zip archive that exsits that the
              provided file offset.  This returns the offset to the
              file in the archive.


Example: This code should unzip all the files in test.zip one by one

int offset=0;

while(1)
{
  offset=kunzip_all("test.zip","/tmp",offset);
  if (offset==-1) break;
}

*/

long kunzip_next(char *zip_filename, char *base_dir, int offset);

/*

kunzip_count_files - Returns the number of files in the zip archive
                     including directories.

Example: count=kunzip_count_files("test.zip");

count will equal the number of files and directories that will be
created if kunzip_all is called.

*/

int kunzip_count_files(char *zip_filename);

/*

kunzip_get_offset_by_number - Returns the offset to the file that is
                      x number of files from the begining. Files in a
                      zip archive start at 0 and end at zip_count_files-1

Example:

kunzip_get_offset_by_number("test.zip",0); // should return an offset of 0
kunzip_get_offset_by_number("test.zip",1); // should return an offset to the
                                              second file in the archive 


*/


int kunzip_get_offset_by_number(char *zip_filename, int file_count);

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

kunzip_get_name - Get the name of the archived file at this offset.

*/

int kunzip_get_name(char *zip_filename, char *name, int offset);

/*

kunzip_get_filesize - Get the filesize of the archived file at this offset.

*/

int kunzip_get_filesize(char *zip_filename, int offset);

/*

kunzip_get_modtime - Get the modified time of the archived file at this offset
                    in time_t format

*/

time_t kunzip_get_modtime(char *zip_filename, int offset);

/*

kunzip_print_version - Print the kunzip library version to the console

*/

int kunzip_print_version();

/*

kunzip_get_version - Get the current kunzip library version.

Example:

  char version[128];
  kunzip_get_version(version);
  printf("%s\n",version);

*/

int kunzip_get_version(char *version_string);




