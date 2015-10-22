#include "libMallocTrace.h"
#include <execinfo.h>

trace_map_t *mallocMap, *mmapMap;

int logFile;
long page_sz;

static char *boot_buf_start, *boot_buf_curr;

FUNCP_DECL(malloc);
FUNCP_DECL(calloc);
FUNCP_DECL(realloc);
FUNCP_DECL(free);
FUNCP_DECL(memalign);
FUNCP_DECL(posix_memalign);
FUNCP_DECL(aligned_alloc);
FUNCP_DECL(valloc);
FUNCP_DECL(malloc_usable_size);
FUNCP_DECL(mmap);
FUNCP_DECL(munmap);
FUNCP_DECL(sbrk);

void log(const char *func, size_t sz);

__attribute__((constructor)) void init() {
	mallocMap = new trace_map_t;
	mmapMap = new trace_map_t;

	page_sz = sysconf(_SC_PAGESIZE);
	if ((logFile = open("logfile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
		printf("error opening file: %s\n", strerror(errno));

	mmap_ptr = (mmap_func *) dlsym(RTLD_NEXT, "mmap");
	munmap_ptr = (munmap_func *) dlsym(RTLD_NEXT, "munmap");

	boot_buf_start = static_cast<char *>((*mmap_ptr)(NULL, 65536, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	boot_buf_curr = boot_buf_start;

	void *lib;
	if(getenv("MALLOC_LIB"))
		lib = dlopen(getenv("MALLOC_LIB"), RTLD_LAZY);
	else
		lib = RTLD_NEXT;

	FUNCP_INIT(malloc);
	FUNCP_INIT(calloc);
	FUNCP_INIT(realloc);
	FUNCP_INIT(free);
	FUNCP_INIT(memalign);
	FUNCP_INIT(posix_memalign);
	FUNCP_INIT(aligned_alloc);
	FUNCP_INIT(valloc);
	FUNCP_INIT(malloc_usable_size);
	FUNCP_INIT(mmap);
	FUNCP_INIT(munmap);
	FUNCP_INIT(sbrk);

	printf("init'd with library %s\n", getenv("MALLOC_LIB"));
}

#define LOG(ptr, sz) if (ptr != NULL && sz > 0) { \
	log("malloc", sz); \
	(*mallocMap)[ptr] = sz; \
}

#define IF_NULL(name, ...) if(name##_ptr == NULL) return reinterpret_cast<name##_func *>(dlsym(RTLD_NEXT, #name))(__VA_ARGS__)

void *malloc(size_t sz) {
	IF_NULL(malloc, sz);
	auto ptr = (*malloc_ptr)(sz);
	LOG(ptr, sz)
	return ptr;
}

void *calloc(size_t num, size_t sz) {
	if (calloc_ptr == NULL) {
		if (!boot_buf_start || boot_buf_curr - boot_buf_start > 65536)
			return NULL;
		//so that dl{open,
		void *ptr = boot_buf_curr;
		boot_buf_curr += num * sz;
		return ptr;
	}

	auto ptr = (*calloc_ptr)(num, sz);
	LOG(ptr, num * sz)
	return ptr;
}

void *realloc(void *ptr, size_t sz) {
	IF_NULL(realloc, ptr, sz);
	log("free", (*mallocMap)[ptr]);
	auto new_ptr = (*realloc_ptr)(ptr, sz);
	if (new_ptr != ptr)
		mallocMap->erase(ptr);
	LOG(new_ptr, sz)
	return new_ptr;
}

void free(void *ptr) {
	if(ptr > boot_buf_start && ptr < boot_buf_curr)
		return;
	IF_NULL(free, ptr);
	size_t sz = (*mallocMap)[ptr];
	if(sz == 0) {
		for (auto it = mallocMap->begin(); it != mallocMap->end(); it++) {
			char *start = static_cast<char *>(it->first);
			if(ptr > start && ptr < start + it->second)
				sz = it->second;
		}
	}
	log("free", sz);
	mallocMap->erase(ptr);
	if(sz)
		(*free_ptr)(ptr);
	else if (ptr) {
		void *array[10];
		int size;

		// get void*'s for all entries on the stack
		size = backtrace(array, 10);
		backtrace_symbols(array, size);
	}
}

void *memalign (size_t align, size_t sz) {
	IF_NULL(memalign, align, sz);
	auto ptr = (*memalign_ptr)(align, sz);
	LOG(ptr, sz)
	return ptr;
}

int posix_memalign (void **memptr, size_t align, size_t sz) {
	IF_NULL(posix_memalign, memptr, align, sz);
	int ret = (*posix_memalign_ptr)(memptr, align, sz);
	LOG(*memptr, sz)
	return ret;
}

void *aligned_alloc(size_t align, size_t sz) {
	IF_NULL(aligned_alloc, align, sz);
	auto ptr = (*aligned_alloc_ptr)(align, sz);
	LOG(ptr, sz)
	return ptr;
}

void *valloc(size_t sz) {
	IF_NULL(valloc, sz);
	auto ptr = (*valloc_ptr)(sz);
	LOG(ptr, sz)
	return ptr;
}

size_t malloc_usable_size(void *ptr) {
	IF_NULL(malloc_usable_size, ptr);
	return (*malloc_usable_size_ptr)(ptr);
}

void *mmap64(void *addr, size_t sz, int prot, int flags, int fd, off64_t offset) {
	printf("mmap64 called\n");
	if(sizeof(off64_t) != sizeof(off_t))
		abort();
	return mmap(addr, sz, prot, flags, fd, offset);
}

void *mmap(void *addr, size_t sz, int prot, int flags, int fd, off_t offset) {
	bool isMyAllocator;
	if ((isMyAllocator = flags & 0x100000))
		flags ^= 0x100000;

	IF_NULL(mmap, addr, sz, prot, flags, fd, offset);

	void *ptr = (*mmap_ptr)(addr, sz, prot, flags, fd, offset);
	if (!isMyAllocator && flags & MAP_ANONYMOUS) {
		sz = (sz + page_sz - 1) & (size_t) ~(page_sz - 1);
		(*mmapMap)[ptr] = sz;
		log("mmap", sz);
	}
	return ptr;
}

int munmap(void *ptr, size_t sz) {
	IF_NULL(munmap, ptr, sz);
	if ((sz = (sz + page_sz - 1) & (size_t) ~(page_sz - 1))) {
		trace_map_t::iterator it;
		size_t old_sz;
		if ((it = mmapMap->find(ptr)) != mmapMap->end()) {
			log("munmap", sz);
			if ((old_sz = it->second) > sz)
				(*mmapMap)[static_cast<char *>(ptr) + sz] = old_sz - sz;
			mmapMap->erase(it);
		} else {
			// if ptr isn't found it might refer to the middle of a previously mmap'd chunk
			// if ptr doesn't refer to a chunk it was probably a file or came from the stl allocator we use for the map
			for (it = mmapMap->begin(); it != mmapMap->end(); it++) {
				char *old_start = static_cast<char *>(it->first);
				//if it does point to the middle of an existing chunk update the mapping
				if (old_start < ptr && old_start + (old_sz = it->second) > ptr) {
					log("munmap", sz);
					it->second = static_cast<char *>(ptr) - old_start;
					//it's technically possible that the munmap divided the chunk
					if (it->second + sz < old_sz)
						(*mmapMap)[static_cast<char *>(ptr) + sz] = old_sz - it->second - sz;
					break;
				}
			}
		}
	}
	return (*munmap_ptr)(ptr, sz);
}

void *sbrk(intptr_t increment) {
	IF_NULL(sbrk, increment);
	auto ptr = (*sbrk_ptr)(increment);
	log("sbrk", increment);
	return ptr;
}

/*
 * logging helper functions
 */
size_t strlen(char *str, size_t len = 37) {
	for (size_t i = 0; i < len; i++)
		if (str[i] == '\0')
			len = i;
	return len;
}

void log(const char *func, size_t sz) {
//	printf("%s %lu", func, sz);
	char buf[37];
	snprintf(buf, 37, "%s %lu\n", func, sz);
	write(logFile, buf, strlen(buf));
}

/*
 * Funcs we don't want to/have not yet replace(d), and hopefully don't have to. We abort because if they are being used we need to implement them
 */

int brk(void *addr) {
	fprintf(stderr, "brk called\n");
	abort();
}

void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */) {
	fprintf(stderr, "mremap called\n");
	abort();
}