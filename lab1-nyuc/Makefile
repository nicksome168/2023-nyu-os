CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyuc

nyuc: nyuc.o argmanip.o

nyuc.o: nyuc.c argmanip.h

argmanip.o: argmanip.c argmanip.h

.PHONY: clean
clean:
	rm -f *.o nyuc

.PHONY: test
test:
	python3 test.py test.txt