#include <map>

#include <dlfcn.h>

#include "wrappers/stlallocator.h"
#include "heaps/top/mmapheap.h"

#ifndef MMAP_ARR_SIZE
#define MMAP_ARR_SIZE 1024
#endif

typedef HL::STLAllocator<pair<const void *, size_t>, HL::MmapHeap> mapAllocator;

class Tracer {
	 void *(*malloc_ptr)(size_t);
	 void (*free_ptr)(void *);
	 void *(*mmap_ptr)(void *, size_t, int, int, int, off_t);
	 int (*munmap_ptr)(void *, size_t);

	 std::map<void *, size_t, less<void *>, mapAllocator> mallocMap, mmapMap;

	 int logFile = 0;

	 size_t strlen(char *str, size_t len = 37) {
		 for (size_t i = 0; i < len; i++)
			 if (str[i] == '\0')
				 len = i;
		 return len;
	 }

	 void log(const char *func, size_t sz) {
		 char buf[37];
		 snprintf(buf, 37, "%s %lu\n", func, sz);
		 write(logFile, buf, strlen(buf));
	 }
public:
	 Tracer() {
		 malloc_ptr = (void *(*)(size_t)) dlsym(getenv("MALLOC_LIB"), "malloc");
		 free_ptr = (void (*)(void *)) dlsym(getenv("MALLOC_LIB"), "free");
		 mmap_ptr = (void *(*)(void *, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
		 munmap_ptr = (int (*)(void *, size_t)) dlsym(RTLD_NEXT, "munmap");

		 if ((logFile = open("logfile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
			 printf("error opening file: %s\n", strerror(errno));

		 printf("tracer init\n");
	 }

	 void *malloc(size_t sz){
		 return (*malloc_ptr)(sz);
	 }
	 void free(void *ptr) {}
	 void *mmap(void *addr, size_t sz, int prot, int flags, int fd, off_t offset) {
		 return (*mmap_ptr)(addr, sz, prot, flags, fd, offset);
	 }
	 int munmap(void *ptr, size_t sz) {
		 return 0;
	 };
};

Tracer *getTracer() {
	static double tBuf[sizeof(Tracer) / sizeof(double) + 1];
	static Tracer *t = new(tBuf) Tracer;
	return t;
}


__attribute__((constructor)) void init() {}

void *malloc(size_t sz) {
	return getTracer()->malloc(sz);
}

void free(void *ptr) {
	getTracer()->free(ptr);
}

void *mmap(void *addr, size_t sz, int prot, int flags, int fd, off_t offset) {
	return getTracer()->mmap(addr, sz, prot, flags, fd, offset);
}

int munmap(void *ptr, size_t sz) {
	return getTracer()->munmap(ptr, sz);
}

//void *malloc(size_t sz) {
//	void *ptr = (*malloc_ptr)(sz);
//	mallocMap[ptr] = sz;
//	log("malloc", sz);
//	return ptr;
//}
//
//void free(void *ptr) {
//	log("free", mallocMap[ptr]);
//	mallocMap.erase(ptr);
//	(*free_ptr)(ptr);
//}
//
//void *mmap(void *addr, size_t sz, int prot, int flags, int fd, off_t offset) {
//	void *ptr = (*mmap_ptr)(addr, sz, prot, flags ^ 0x100, fd, offset);
//	if (MAP_ANONYMOUS & flags && !(flags & 0x100)) {
//		log("mmap", sz);
//		mmapArr[arrPtr++] = (size_t) ptr;
//	}
//	return ptr;
//}
//
//int munmap(void *ptr, size_t sz) {
//	for (int i = 0; i < MMAP_ARR_SIZE; ++i)
//		if (mmapArr[i] == (size_t) ptr)
//			log("munmap", sz);
//	return (*munmap_ptr)(ptr, sz);
//}