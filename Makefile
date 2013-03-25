BINDIR=~/bin
INC=
LDFLAGS=
INSTALL=cp
CC = g++
FLAGS = -Wall -g -O3 -Wextra -fopenmp -D NDEBUG -march=native -licuuc
TARGET = ngram.counting
OBJS = ngram.counting.o

default: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) 

.SUFFIXES:
.SUFFIXES: .o .cpp

.PHONY: clean test

.cpp.o:
	$(CC) $(FLAGS) -o $@ -c $< $(INC)

clean:
	rm -f *.o test/*.o *~ ngram.counting

