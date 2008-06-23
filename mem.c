/*
 * mem.c: memory allocation wrappers with a few simple checks
 *
 * Copyright (c) 2002-2008 Dennis Stosberg <dennis@stosberg.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2 as published by the Free Software Foundation
 */

#include "mem.h"

#ifdef MEMDEBUG
static void    meminfo_add(void *p, size_t size, const char *file, int line);
static void    meminfo_rm(void *p, const char *file, int line);
static MEMINFO *meminfo_getinfo(void *p);
static void    print_memory_stats(void);
static void    die(const char *format, ...);
static void    warn(const char *format, ...);

static size_t  mem_num_malloced    = 0;
static size_t  mem_bytes_malloced  = 0;

static size_t  mem_num_freed       = 0;
static size_t  mem_bytes_freed     = 0;

static MEMINFO **meminfo = NULL;
static size_t  meminfo_size  = 0;
static size_t  meminfo_count = 0;

static unsigned char magic[] = { 0xFE, 0xDC, 0xBA, 0x98,
				 0x76, 0x54, 0x32, 0x10 };

/**
 *  Adds information about an newly allocated memory region to the
 *  meminfo structure.
 */
static void meminfo_add(void *p, size_t size, const char *file, int line) {

	if(!meminfo)
		if(atexit(print_memory_stats))
			warn("Memory statistics will not be shown.");

	/* enlarge MEMINFO structure if necessary */
	if(meminfo_count == meminfo_size) {
		meminfo_size = meminfo_size ? meminfo_size << 1 : 1;
		meminfo = realloc(meminfo, meminfo_size * sizeof(MEMINFO*));
	}

	/* add information to structure */
	meminfo[meminfo_count] = malloc(sizeof(MEMINFO));
	meminfo[meminfo_count]->addr = p;
	meminfo[meminfo_count]->size = size;
	meminfo[meminfo_count]->file = file;
	meminfo[meminfo_count]->line = line;
	meminfo_count++;
#ifdef MEMINFO_VERBOSE
	printf("allocated 0x%x at %s:%d\n", p, file, line);
#endif
	mem_bytes_malloced += size;
	mem_num_malloced++;
}


/**
 *  Removes information about a just freed memory region to the
 *  meminfo structure.  Halts the program if the region to be freed
 *  has never been allocated at all.
 */
static void meminfo_rm(void *p, const char *file, int line) {
	size_t i;

	for(i = 0; i < meminfo_count; ++i) {
		if(p == meminfo[i]->addr) {
#ifdef MEMINFO_VERBOSE
			fprintf(stderr, "freed 0x%x at %s:%d\n",
				p, file, line);
			fprintf(stderr, "  was allocated at %s:%d\n",
				meminfo[i]->file, meminfo[i]->line);
#endif
			mem_bytes_freed += meminfo[i]->size;
			free(meminfo[i]);
			meminfo[i] = meminfo[meminfo_count - 1];
			meminfo_count--;
			mem_num_freed++;
			return;
		}
	}

	die("Tried to free a piece of memory at 0x%p in %s:%d"
	    " which is not allocated", (void*)p, file, line);
}


/**
 *  Returns the size of a malloc'ed region.
 */
static MEMINFO *meminfo_getinfo(void *p) {
	size_t i;

	for(i = 0; i < meminfo_count; ++i)
		if(p == meminfo[i]->addr)
			return(meminfo[i]);
	return(0);
}


/**
 *  Prints statistics about allocated and deallocated memory to
 *  stderr just before the program exits.  If there are memory
 *  regions left that were allocated but never freed, it will print
 *  information on those regions as well.
 */
static void print_memory_stats(void) {
	size_t i;

	if(mem_num_malloced == mem_num_malloced
	   && mem_bytes_malloced == mem_bytes_freed)
		return;

	fprintf(stderr, "Memory statistics: \n"
		"  %lu allocations   (%lu bytes)\n"
		"  %lu deallocations (%lu bytes)\n"
		"  %ld difference    (%ld bytes)\n",
		(unsigned long)mem_num_malloced,
		(unsigned long)mem_bytes_malloced,
		(unsigned long)mem_num_freed,
		(unsigned long)mem_bytes_freed,
		(long)(mem_num_malloced - mem_num_freed),
		(long)(mem_bytes_malloced - mem_bytes_freed)
		);

	if(meminfo_count) {
		fprintf(stderr, "%lu malloc'ed regions were never "
			"free'd:\n", (unsigned long)meminfo_count);
		for(i = 0; i < meminfo_count; ++i) {
			fprintf(stderr, "  %p (%lu bytes) allocated at %s:%d\n",
				(void *)meminfo[i]->addr,
				(unsigned long)meminfo[i]->size,
				meminfo[i]->file, meminfo[i]->line);
			free(meminfo[i]);
		}
		free(meminfo);
	}
}


/**
 *  Allocates memory and records information on the allocated
 *  memory.  It will halt the program if the memory region to be
 *  allocated has a size of zero.
 */
void *ymalloc_dbg(size_t size, const char *file, int line) {
	void *p;

	if(!size)
		die("Trying to allocate 0 bytes at %s:%d", file, line);

	p = malloc(size + 2*sizeof(magic));
	if(!p)
		die("Out of memory at %s:%d while trying to allocate %lu bytes",
		    file, line, (unsigned long)size);

	meminfo_add(p, size, file, line);

	memcpy(p, &magic, sizeof(magic));
	memcpy((char*)p + size + sizeof(magic), &magic, sizeof(magic));

	return((char*)p + sizeof(magic));
}


/**
 *  Frees memory and removes information on the memory region. Halts
 *  the program, if the region to be freed has never been allocated
 *  at all.
 */
void yfree_dbg(void *p, const char *file, int line) {
	MEMINFO *info;

	if(!p)
		die("Trying to free null pointer at %s:%d", file, line);

	info = meminfo_getinfo((char*)p - sizeof(magic));

	if(info == NULL) {
		die("Looks like we were asked to free a piece of memory"
		    " that we never allocated: %s:%d", file, line);
	}

	/* Check, if region boundaries are intact. */
	if(memcmp((char*)p - sizeof(magic), &magic, sizeof(magic))
	   || memcmp((char*)p + info->size, &magic, sizeof(magic))) {
		die("The boundaries of a region allocated at %s:%d were "
		    "overwritten.", info->file, info->line);
	}

	free((char*)p - sizeof(magic));
	meminfo_rm((char*)p - sizeof(magic), file, line);

	p = NULL;
}


/**
 *  Allocates memory and records information on the allocated
 *  memory.  This function will halt the program if the memory
 *  region to be allocated has a size of zero.
 */
void *ycalloc_dbg(size_t number, size_t size, const char *file, int line) {
	void *p;
	p = ymalloc_dbg(number * size, file, line);
	memset(p, 0, number * size);
	return(p);
}


/**
 *  Reallocates memory and updates the information on the memory
 *  region.  This function will halt the program if new size of the
 *  memory region is zero ot the region has never been allocated
 *  with ymalloc or ycalloc.
 */
void *yrealloc_dbg(void *p, size_t size, const char *file, int line) {
	MEMINFO *area_info;

	if(!size) {
		die("Trying to reallocate 0 bytes at %s:%d", file, line);
	}

	if(!p) {
		p = ymalloc_dbg(size, file, line);
		return(p);
	}

	/* Check, if region boundaries are intact. */
	area_info = meminfo_getinfo((char*)p - sizeof(magic));
	if(memcmp((char*)p - sizeof(magic), &magic, sizeof(magic)) ||
	   memcmp((char*)p + area_info->size, &magic, sizeof(magic))) {
		die("The boundaries of a region allocated at %s:%d were "
		    "overwritten.", area_info->file, area_info->line);
	}

	meminfo_rm((char*)p - sizeof(magic), file, line);
	p = realloc((char*)p - sizeof(magic), size + 2*sizeof(magic));

	if(!p)
		die("Out of memory at %s:%d while trying to allocate %lu bytes",
		    file, line, (unsigned long)size);

	/* realloc has copied the magic at the beginning of the
	   memory area.  We need to set the final magic only    */
	memcpy((char*)p + size + sizeof(magic), &magic, sizeof(magic));
	meminfo_add(p, size, file, line);

	return((char*)p + sizeof(magic));
}


/**
 *  Prints an error message to stdout and halts the program
 */
static void warn(const char *format, ...) {
	va_list argp;
	va_start(argp, format);
	vfprintf(stderr, format, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}

/**
 *  Prints an error message to stdout and halts the program
 */
static void die(const char *format, ...) {
	va_list argp;
	va_start(argp, format);
	vfprintf(stderr, format, argp);
	va_end(argp);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

#endif
