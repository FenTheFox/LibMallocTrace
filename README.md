# LibMallocTrace
A simple library that traces all malloc/free and mmap/munmap calls.

# Usage
LD_PRELOAD=<path to library>/libmalloctrace.so <application to trace>
If you want to also use a spesific malloc library set MALLOC_LIB to the path of that library.