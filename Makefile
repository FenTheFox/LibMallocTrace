CPPFLAGS=-g -O3 -Wall -std=c++11
LIBFLAGS=$(CPPFLAGS) -IHeap-Layers -ldl -shared -fPIC

all: setup tester libmalloctrace.so

setup:
	mkdir -p bin

tester: main.cpp
	g++ -o bin/$@ $(CPPFLAGS) $^

libmalloctrace.so: libMallocTrace.cpp
	g++ -o bin/$@ $(LIBFLAGS) $^