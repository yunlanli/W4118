CC = gcc
CXX = g++

INCLUDES = -lm
CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

.PHONY: all clean

all: clean get_pgtbl_info pgtbl_sample

get_pgtbl_info: expose_pgtbl.h

pgtbl_sample: expose_pgtbl.h

clean:
	rm -rf *.o get_pgtbl_info
