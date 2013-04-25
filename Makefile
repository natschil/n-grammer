BINDIR=~/bin
INC=

CC=gcc
CFLAGS=

CPP=g++
CPPLFAGS=-Wall -O3 -I/usr/include -ldl

LDFLAGS=

all: ngram.analysis
OBJECTS=main.o invert_index.o manage_metadata.o

ngram.analysis : $(OBJECTS)
	$(CPP) $(CPPFLAGS)-o $@ $(OBJECTS) $(LIBS) $(LDFLAGS)

.PHONY: clean test

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $< $(INC)


clean:
	rm -rf ./*.o ngram.analysis
