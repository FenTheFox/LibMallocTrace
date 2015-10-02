#include <map>

#include <dlfcn.h>
#include <slang.h>

#include "wrappers/stlallocator.h"
#include "heaps/top/mmapheap.h"

#ifndef MMAP_ARR_SIZE
#define MMAP_ARR_SIZE 1024
#endif

void *(*malloc_ptr)(size_t) = (void *(*)(size_t)) dlsym(RTLD_NEXT, "malloc");
void (*free_ptr)(void *) = (void (*)(void *)) dlsym(RTLD_NEXT, "free");
void *(*mmap_ptr)(void *, size_t, int, int, int, off_t) = NULL;
int (*munmap_ptr)(void *, size_t) = NULL;

typedef HL::STLAllocator<pair<const void *, size_t>, HL::MmapHeap> mapAllocator;
std::map<void *, size_t, less<void *>, mapAllocator> mallocMap;

size_t *mmapArr = NULL, arrPtr = 0;
int logFile = 0;

__attribute__((constructor)) void init() {
	mmap_ptr = (void *(*)(void *, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
	munmap_ptr = (int (*)(void *, size_t)) dlsym(RTLD_NEXT, "munmap");
	mmapArr = (size_t *) (*mmap_ptr)(NULL, MMAP_ARR_SIZE * sizeof(size_t), PROT_READ | PROT_WRITE,
	                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	printf("init\n");
	logFile = open("logfile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(logFile < 0)
		printf("error opening file: %s\n", strerror(errno));
}

size_t strlen(char *str, size_t len = 37)
{
	for (int i = 0; i < len; i++)
		if (str[i] == '\0')
			len = i;
	return len;
}

void log(const char * func, size_t sz) {
	char buf[37];
	snprintf(buf, 37, "%s %lu\n", func, sz);
	write(logFile, buf, strlen(buf));
}

void *malloc(size_t sz) {
	void *ptr = (*malloc_ptr)(sz);
	mallocMap[ptr] = sz;
	log("malloc", sz);
	return ptr;
}

void free(void *ptr) {
	log("free", mallocMap[ptr]);
	mallocMap.erase(ptr);
	(*free_ptr)(ptr);
}

void *mmap(void *addr, size_t sz, int prot, int flags, int fd, off_t offset) {
	void *ptr = (*mmap_ptr)(addr, sz, prot, flags, fd, offset);
	if (MAP_ANONYMOUS & flags && !(flags & 0x100)) {
		log("mmap", sz);
		mmapArr[arrPtr++] = (size_t) ptr;
	}
	return ptr;
}

int munmap(void *ptr, size_t sz) {
	for (int i = 0; i < MMAP_ARR_SIZE; ++i)
		if (mmapArr[i] == (size_t) ptr)
			log("munmap", sz);
	return (*munmap_ptr)(ptr, sz);
}