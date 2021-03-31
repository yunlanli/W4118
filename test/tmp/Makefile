CC = gcc
CXX = g++

INCLUDES =
CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

.PHONY: all clean

all: clean mm_assignment get_pgtbl_info pgtbl_sample

mm_assignment: expose_pgtbl.h

get_pgtbl_info: expose_pgtbl.h

pgtbl_sample: expose_pgtbl.h

clean:
	rm -rf *.o mm_assignment get_pgtbl_info
