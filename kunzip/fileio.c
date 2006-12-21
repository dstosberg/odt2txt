#include <stdio.h>
#include <stdlib.h>

/*

This code is Copyright 2005-2006 by Michael Kohn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2 as published by the Free Software Foundation

*/

int read_int(FILE *in)
{
int c;

  c=getc(in);
  c=c|(getc(in)<<8);
  c=c|(getc(in)<<16);
  c=c|(getc(in)<<24);

  return c;
}

int read_word(FILE *in)
{
int c;

  c=getc(in);
  c=c|(getc(in)<<8);

  return c;
}

int read_chars(FILE *in, char *s, int count)
{
int t;

  for (t=0; t<count; t++)
  {
    s[t]=getc(in);
  }

  s[t]=0;

  return 0;
}

int read_int_b(FILE *in)
{
int c;

  c=getc(in);
  c=(c<<8)+getc(in);
  c=(c<<8)+getc(in);
  c=(c<<8)+getc(in);

  return c;
}

int read_word_b(FILE *in)
{
int c;

  c=getc(in);
  c=(c<<8)+getc(in);

  return c;
}

int read_buffer(FILE *in, unsigned char *buffer, int len)
{
int t;

  t=0;
  while (t<len)
  {
    t=t+fread(buffer+t,1,len-t,in);
  }

  return t;
}

