CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o cmdreader.o

nyush.o: nyush.c

cmdreader.o: cmdreader.c cmdreader.h

.PHONY: clean
clean:
	rm -f *.o nyush

.PHONY: test
test:
	tar xf nyush-autograder.tar.xz
	zip nyush.zip Makefile *.h *.c
	cp nyush.zip nyush-autograder

.PHONY: zip
zip:
	zip nyush.zip Makefile *.h *.c