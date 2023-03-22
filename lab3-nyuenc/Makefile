CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o

nyuenc.o: nyuenc.c

.PHONY: clean
clean:
	rm -f *.o nyuenc

.PHONY: test
test:
	tar xf nyuenc-autograder.tar.xz
	zip nyuenc.zip Makefile *.h *.c
	cp nyuenc.zip nyuenc-autograder

.PHONY: zip
zip:
	zip nyuenc.zip Makefile *.h *.c