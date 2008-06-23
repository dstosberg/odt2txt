/*
 * mem.c: memory allocation wrappers with a few simple checks
 *
 * Copyright (c) 2002-2008 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#ifndef MEM_H
#define MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef MEMDEBUG

/**
 * A container to keep the information about allocated memory.
 */
typedef struct {
	void       *addr;
	size_t     size;
	const char *file;
	int        line;
} MEMINFO;

#define yfree(p) yfree_dbg(p, __FILE__, __LINE__)
void yfree_dbg(void *p, const char *file, int line);

#define ymalloc(size) ymalloc_dbg(size, __FILE__, __LINE__)
void *ymalloc_dbg(size_t size, const char *file, int line);

#define ycalloc(num, size) ymalloc_dbg(num, size, __FILE__, __LINE__)
void *ycalloc_dbg(size_t number, size_t size, const char *file, int line);

#define yrealloc(p, size) yrealloc_dbg(p, size, __FILE__, __LINE__)
void *yrealloc_dbg(void *p, size_t size, const char *file, int line);

#else
#define yfree(p)           free(p)
#define ymalloc(size)      malloc(size)
#define ycalloc(num, size) calloc(num, size);
#define yrealloc(p, size)  realloc(p, size);
#endif

#endif /* MEM_H */
