CC = gcc
CFLAGS = -g -Wall

test:
	$(CC) $(CFLAGS) test.c -o test

.PHONY : clean
clean: 
	rm test