CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc

all: v2.o data_utils.o

# unit tests for the libraries
test: data_utils.o
	$(CC) $(CFLAGS) -DTEST v2_utils.c $^ -o test

v2.o:
	$(CC) $(CFLAGS) v2_utils.c -c

data_utils.o:
	$(CC) $(CFLAGS) data_utils.c -c

clean:
	rm -f core
	rm -f *~ *.o
	rm -f test
	rm -f tt.gif