CPPFLAGS=-g -O2 -std=c++11
LIBFLAGS=-IHeap-Layers -ldl -shared -fPIC $(CPPFLAGS)

all: test libmalloctrace.so

test: main.cpp
	g++ -o bin/$@ $(CPPFLAGS) $^

libmalloctrace.so: libMallocTrace.cpp
	g++ -o bin/$@ $(LIBFLAGS) $^