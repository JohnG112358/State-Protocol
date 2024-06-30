CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make
INCLUDES = -I ../lib

all: udp tcp

udp: ../lib/v2_utils.o ../lib/data_utils.o
	$(CC) $(CFLAGS) udpserv2.c $^ $(INCLUDES) -o udpserv2

tcp: ../lib/v2_utils.o ../lib/data_utils.o
	$(CC) $(CFLAGS) tcpserv2.c $^ $(INCLUDES) -o tcpserv2

../lib/v2_utils.o:
	$(MAKE) v2_utils.o --directory=../lib

../lib/data_utils.o:
	$(MAKE) data_utils.o --directory=../lib

clean:
	rm -f *~ *.o
	rm -f tcpserv2
	rm -f udpserv2
	rm -f tt.gif
	$(MAKE) -C ../lib clean