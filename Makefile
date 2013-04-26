BINDIR=~/bin
INC=

CC=gcc
CFLAGS= -g

CPP=g++
CPPFLAGS= -Wall -O0 -I/usr/include -ldl -D_FILE_OFFSET_BITS=64 -g

LDFLAGS=

all: ngram.analysis
OBJECTS=main.o invert_index.o manage_metadata.o search.o

ngram.analysis : $(OBJECTS)
	$(CPP) $(CPPFLAGS) -o $@ $(OBJECTS) $(LIBS) $(LDFLAGS) 

.PHONY: clean test

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $< $(INC)


clean:
	rm -rf ./*.o ngram.analysis
