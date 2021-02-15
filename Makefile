CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

test: test.c

.PHONY: clean
clean:
		rm -f *.o *~ a.out core test

.PHONY: all
all: clean test

