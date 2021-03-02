CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean tmp

.PHONY: tmp
tmp: tmp/Makefile tmp/syscall_test.c
	cd tmp && make

.PHONY: clean
clean:
		rm -f *.o *~



