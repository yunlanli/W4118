CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean syscall_test tmp

syscall_test: syscall_test.c

.PHONY: tmp
tmp: tmp/Makefile tmp/syscall_test.c tmp/setscheduler_test.c tmp/setwrr.c tmp/setwrr2.c
	cd tmp && make

.PHONY: clean
clean:
		rm -f *.o *~ syscall_test



