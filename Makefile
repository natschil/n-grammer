BINDIR=~/bin
INC=

CC=gcc
CFLAGS= -g

CPP=g++
CPPFLAGS= -Wall -O3 -I/usr/include -ldl -D_FILE_OFFSET_BITS=64 -g -std=c++11

LDFLAGS=

all: ngram.analysis
OBJECTS=main.o invert_index.o manage_metadata.o search.o get_top.o make_wordlength_stats.o view_wordlength_stats.o searchable_file.o

ngram.analysis : $(OBJECTS)
	$(CPP) $(CPPFLAGS) -o $@ $(OBJECTS) $(LIBS) $(LDFLAGS) 

.PHONY: clean test

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $< $(INC)


clean:
	rm -rf ./*.o ngram.analysis
