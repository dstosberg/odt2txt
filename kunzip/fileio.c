#include <stdio.h>
#include <stdlib.h>

/*

This code is Copyright 2005-2006 by Michael Kohn

This package is licensed under the LGPL. You are free to use this library
in both commercial and non-commercial applications as long as you dynamically
link to it. If you statically link this library you must also release your
software under the LGPL. If you need more flexibility in the license email
me and we can work something out. 

Michael Kohn <mike@mikekohn.net>

*/

int write_int(FILE *out, int n)
{
  putc((n&255),out);
  putc(((n>>8)&255),out);
  putc(((n>>16)&255),out);
  putc(((n>>24)&255),out);

  return 0;
}

int write_word(FILE *out, int n)
{
  putc((n&255),out);
  putc(((n>>8)&255),out);

  return 0;
}

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

int write_chars(FILE *out, char *s)
{
int t;

  t=0;
  while(s[t]!=0 && t<255)
  {
    putc(s[t++],out);
  }

  return 0;
}

int write_int_b(FILE *out, int n)
{
  putc(((n>>24)&255),out);
  putc(((n>>16)&255),out);
  putc(((n>>8)&255),out);
  putc((n&255),out);

  return 0;
}

int write_word_b(FILE *out, int n)
{
  putc(((n>>8)&255),out);
  putc((n&255),out);

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

int write_buffer(FILE *out, unsigned char *buffer, int len)
{
int t;

  t=0;
  while (t<len)
  {
    t=t+fwrite(buffer+t,1,len-t,out);
  }

  return t;
}



