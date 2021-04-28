CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean lifecycle tracker

lifecycle:

tracker:

tmp: tmp/Makefile tmp/test tmp/test2 tmp/test3 tmp/generator
	cd tmp && make

.PHONY: clean
clean:
	rm -f *.o *~ tmp/syscall_test tmp/test tmp/test2 \
	    tmp/test3 tmp/generator lifecycle tracker



