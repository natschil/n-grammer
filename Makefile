BINDIR=~/bin
INC=
LDFLAGS=`icu-config --ldflags  --ldflags-icuio` 
INSTALL=cp
CC = g++
FLAGS = -Wall -g -O3 -Wextra -fopenmp -D NDEBUG -march=native  -I/usr/include -ldl -lm `icu-config --cppflags` 
TARGET = ngram.counting
OBJS = ngram.counting.o main.o

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

