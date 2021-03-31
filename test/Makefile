CC = gcc
CXX = g++

INCLUDES =
CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

.PHONY: all clean

all: clean vm_inspector

vm_inspector: vm_inspector.c

clean:
	rm -rf *.o vm_inspector
