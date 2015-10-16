set breakpoint pending on

set exec-wrapper env LD_PRELOAD=./bin/libmalloctrace.so
file bin/tester
b libMallocTrace.cpp:79
r