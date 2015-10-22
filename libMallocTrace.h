//
// Created by tallman on 10/15/15.
//

#ifndef LIBMALLOCTRACE_LIBMALLOCTRACE_H
#define LIBMALLOCTRACE_LIBMALLOCTRACE_H

#include <map>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include "wrappers/stlallocator.h"
#include "heaps/top/mmapheap.h"

#ifndef MMAP_ARR_SIZE
#define MMAP_ARR_SIZE 1024
#endif

#define FUNCP_DECL(x) x##_func *x##_ptr
#define FUNCP_INIT(x) if((x##_ptr = reinterpret_cast<x##_func *> (dlsym(lib, #x))) == NULL) printf("%s\n", dlerror())

typedef HL::STLAllocator<pair<const void *, size_t>, HL::MmapHeap> MyAllocator;
typedef std::map<void *, size_t, less<void *>, MyAllocator> trace_map_t;

typedef void * malloc_func             (size_t);
typedef void * calloc_func             (size_t, size_t);
typedef void * realloc_func            (void *, size_t);
typedef void   free_func               (void *);
typedef void * memalign_func           (size_t, size_t);
typedef int    posix_memalign_func     (void **, size_t, size_t);
typedef void * aligned_alloc_func      (size_t, size_t);
typedef void * valloc_func             (size_t);
typedef size_t malloc_usable_size_func (void *);

typedef void * mmap_func               (void *, size_t, int, int, int, off_t);
typedef int    munmap_func             (void *, size_t);
typedef void * sbrk_func               (intptr_t);
#endif //LIBMALLOCTRACE_LIBMALLOCTRACE_H
