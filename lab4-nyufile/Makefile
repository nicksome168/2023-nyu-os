CC=gcc
CFLAGS=-O3 -g -pedantic -std=gnu17 -Wall -Werror -Wextra
LDFLAGS=-O3 -pthread

.PHONY: all
all: nyufile

nyufile: nyufile.o

nyufile.o: nyufile.c

.PHONY: clean
clean:
	rm -f *.o nyufile

.PHONY: test
test:
	zip nyufile.zip Makefile *.h *.c
	cp nyufile.zip nyufile-autograder

.PHONY: zip
zip:
	zip nyufile.zip Makefile *.h *.c