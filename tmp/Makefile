CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean test generator test2 test3

test: test.c

generator: generator.c

test2: test2.c

test3: test3.c

.PHONY: clean
clean:
		rm -f *.o *~ test generator test2 test3
