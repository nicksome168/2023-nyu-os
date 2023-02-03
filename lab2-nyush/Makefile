CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o cmdreader.o

nyush.o: nyush.c

cmdreader.o: cmdReader.c cmdReader.h

.PHONY: clean
clean:
	rm -f *.o nyush

.PHONY: test
test:
	python3 test.py test.txt