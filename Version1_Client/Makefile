CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make
INCLUDES = -I ../lib

all: ../lib/data_utils.o
	$(CC) $(CFLAGS) tcpcli.c $^ $(INCLUDES) -o client

ec: ../lib/data_utils.o
	$(CC) $(CFLAGS) tcpcli_ec.c $^ $(INCLUDES) -o clientec

../lib/data_utils.o:
	$(MAKE) data_utils.o --directory=../lib

test: all
	./test.sh

clean:
	rm -f *~ *.o
	rm -f *~ *.gif
	rm -f core
	rm -f client
	rm -f clientec
	$(MAKE) -C ../lib clean