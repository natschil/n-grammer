BINDIR=~/bin
INC=
LDFLAGS=`icu-config --ldflags  --ldflags-icuio` 
INSTALL=cp
CPP = g++
CC = gcc
FLAGS = -Wall -g -O3 -Wextra -fopenmp -D NDEBUG -march=native  -I/usr/include -ldl -lm `icu-config --cppflags`  -pg
CFLAGS = -Wall -g -O3 -Wextra -fopenmp -D NDBUG -march=native -I/usr/include -ldl -lm `icu-config --cflags` -pg

TARGET = ngram.counting
MERGER = merger
OBJS = ngram.counting.o main.o
MERGER_OBJS = mergefiles.o merge_files.o

default: $(TARGET) $(MERGER)
$(TARGET): $(OBJS)
	$(CPP) $(FLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) 
$(MERGER) :  $(MERGER_OBJS)
	$(CC) $(FLAGS) -o $@ $(MERGER_OBJS) $(LDFLAGS)


.PHONY: clean test

.cpp.o:
	$(CPP) $(FLAGS) -o $@ -c $< $(INC)
.c.o:
	$(CC) $(FLAGS) -o $@ -c $< $(INC)

clean:
	rm -f *.o test/*.o *~ ngram.counting

