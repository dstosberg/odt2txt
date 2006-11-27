#include <stdio.h>
#include <stdlib.h>

#include "fileio.h"
#include "kinflate.h"
#include "kunzip.h"
#include "zipfile.h"

/*

This code is Copyright 2005-2006 by Michael Kohn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2 as published by the Free Software Foundation

*/

int main(int argc, char *argv[])
{
int i;

  printf("\nkunzip ZIP decompression routines\n");
  printf(COPYRIGHT"\n");
  printf(VERSION"\n\n");

  if (argc!=2)
  {
    printf("Usage: kunzip <infile>\n");
    exit(0);
  }

  kunzip_inflate_init();

  i=kunzip_count_files(argv[1]);
  printf("There are %d files in this archive.\n",i);
  if (i>0)
  {
    // i=kunzip_all(argv[1],"/tmp/kunziptest");
    printf("Offset = %d\n",kunzip_get_offset_by_name(argv[1],"support/adobe",0,-1));
  }

  kunzip_inflate_free();

  if (i!=0)
  {
    printf("Problem decompressing\n");
  }

  return 0;
}



