BINDIR=~/bin
INC=
LDFLAGS=-lunistring -ldl -lm
INSTALL=cp
CPP = g++
CC = gcc
FLAGS = -Wall -Wno-pointer-arith -g -O3 -fopenmp -Wextra -D NDEBUG -march=native  -I/usr/include -D_FILE_OFFSET_BITS=64  -std=c++11
CFLAGS = -Wall -g -O3 -fopenmp -Wextra -D NDEBUG -march=native  -I/usr/include  -D_FILE_OFFSET_BITS=64

TARGET = ngram.counting
OBJS = ngram.counting.o main.o  mergefiles.o searchindexcombinations.o indices_v2.o util.o words.o manage_metadata.o
MERGER = merger
SORTED_CHECK = check_if_sorted

MERGER_OBJS = mergefiles.o merge_files.o util.o

default: $(TARGET) $(MERGER) $(SORTED_CHECK)
$(TARGET): $(OBJS)
	$(CPP) $(FLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) 
$(MERGER) :  $(MERGER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MERGER_OBJS) $(LDFLAGS)

$(SORTED_CHECK) : check_if_sorted.o
	$(CC) $(CFLAGS) -o $@ check_if_sorted.o

.PHONY: clean test

.cpp.o:
	$(CPP) $(FLAGS) -o $@ -c $< $(INC)
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $< $(INC)

clean:
	rm -f *.o test/*.o *~ ngram.counting
test:
	./run_largescale_tests.sh ./corpora/very_tiny
	./run_largescale_tests.sh ./corpora/pos_very_very_tiny

