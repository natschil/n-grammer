BINDIR=~/bin
INC=

CC=gcc
CFLAGS=

CPP=g++
CPPLFAGS=-Wall -O3 -I/usr/include -ldl

LDFLAGS=

all: ngram.analysis

ngram.analysis : main.o
	$(CPP) $(CPPFLAGS)-o $@ main.o $(LIBS) $(LDFLAGS)

.PHONY: clean test

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $< $(INC)


clean:
	rm -rf ./*.o ngram.analysis
