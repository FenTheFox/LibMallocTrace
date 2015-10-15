CPPFLAGS=-g -O3 -Wall -std=c++11
LIBFLAGS=$(CPPFLAGS) -IHeap-Layers -ldl -shared -fPIC

all: tester libmalloctrace.so

.setup:
	mkdir -p bin
	cd Heap-Layers && git apply ../0001-Add-a-way-to-destinguish-HL-mmap-calls-from-applicat.patch
	cd ..
	touch .setup

tester: .setup main.cpp
	g++ -o bin/$@ $(CPPFLAGS) main.cpp

libmalloctrace.so: .setup libMallocTrace.cpp
	g++ -o bin/$@ $(LIBFLAGS) libMallocTrace.cpp

test: tester libmalloctrace.so
	LD_PRELOAD=./bin/libmalloctrace.so bin/tester

clean:
	rm bin/*