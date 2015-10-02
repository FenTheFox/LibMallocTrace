//
// Created by timm on 10/1/15.
//

#include <stdlib.h>
#include <cmath>
#include <vector>
#include <stdio.h>

//#define MMAP(x) mmap(NULL, x, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)

size_t rand(size_t max) {
	return (size_t) (floor((max + 1) * rand() / RAND_MAX));
}

int main() {
	std::vector<char *> v;
	printf("staring allocs");
	for (int i = 0; i < 100; ++i)
		v.push_back((char *)malloc(rand(4096)));

	for (int i = 0; i < 2048; ++i) {
		if (rand(1))
			v.push_back((char *)malloc(rand(4096)));
		else {
			int idx = rand(v.size() - 1);
			free(v[idx]);
			v.erase(v.begin() + idx);
		}
	}
}