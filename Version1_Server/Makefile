CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make
INCLUDES = -I ../lib

all: udp tcp

udp: ../lib/data_utils.o
	$(CC) $(CFLAGS) udpserv.c $^ $(INCLUDES) -o udpserv

tcp: ../lib/data_utils.o
	$(CC) $(CFLAGS) tcpserv.c $^ $(INCLUDES) -o tcpserv

../lib/data_utils.o:
	$(MAKE) data_utils.o --directory=../lib

clean:
	rm -f *~ *.o
	rm -f *~ *.gif
	rm -f core
	rm -f tcpserv
	rm -f udpserv
	$(MAKE) -C ../lib clean