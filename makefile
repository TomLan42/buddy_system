.PHONY: build
build:
	gcc -Wall -g main.c allocator.c lru.c util.c -o main

.PHONY: clean
clean:
	rm main