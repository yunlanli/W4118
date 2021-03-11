CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean syscall_test wrr1_test setscheduler_test

syscall_test: syscall_test.c

wrr1_test: wrr1_test.c

setscheduler_test: setscheduler_test.c

.PHONY: clean
clean:
		rm -f *.o *~ syscall_test wrr1_test setscheduler_test



