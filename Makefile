CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: all
all: clean test generator test2 test3 lifecycle tracker

tracker: tracker.c

gen: gen.c

.PHONY: clean
clean:
		rm -f *.o *~ a.out core test generator test3 lifecycle tracker gen test2

test: test.c

generator: generator.c

test2: test2.c

test3: test3.c

lifecycle: lifecycle.c


