BINDIR=~/bin
INC=
LDFLAGS=`icu-config --ldflags  --ldflags-icuio` 
INSTALL=cp
CPP = g++
CC = gcc
FLAGS = -Wall -g -O0 -Wextra  -D NDEBUG -march=native  -I/usr/include -ldl -lm `icu-config --cppflags` 
CFLAGS = -Wall -g -O3 -Wextra -fopenmp -D NDEBUG -march=native  -I/usr/include -ldl -lm `icu-config --cppflags`

TARGET = ngram.counting
OBJS = ngram.counting.o main.o memory_management.o mergefiles.o
MERGER = merger

MERGER_OBJS = mergefiles.o merge_files.o

default: $(TARGET) $(MERGER)
$(TARGET): $(OBJS)
	$(CPP) $(FLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) 
$(MERGER) :  $(MERGER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MERGER_OBJS) $(LDFLAGS)






.PHONY: clean test

.cpp.o:
	$(CPP) $(FLAGS) -o $@ -c $< $(INC)
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $< $(INC)

clean:
	rm -f *.o test/*.o *~ ngram.counting

