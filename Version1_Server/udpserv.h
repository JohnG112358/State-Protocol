#ifndef UDPSERV
#define UDPSERV

#include <stdint.h>

#define SERV_HOST_ADDR  "127.0.0.1" // address for the host
#define QUERY_LENGTH  4 // by the version 1 spec, the query must be 4 bytes
#define HEADER_LENGTH 6 // by the version 1 spec, the header must be 6 bytes
// udp datagrams can have 576 bytes in total to guarantee no fragmentation 
// https://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
// 500 bytes gives us extra leeway while not prohibiting functionality we might gain by including slightly more bytes
// Note this means we cannot send the gifs over udp, as they are all 1kb or more
#define MAX_PACK_LEN 500 

// represents a request to the server
struct request {
     uint8_t  version;
     uint8_t opcode;
     char char1;
     char char2;
}__attribute__((packed));

// represents the server's response to a query
struct response{
     uint8_t  version;
     uint8_t  status;
     uint32_t len;
     unsigned char payload[MAX_PACK_LEN];
}__attribute__((packed));

/*
Parses a query to the server and sends the response to the client
Args:
- client_socket: The socket to read/write from
Returns:
- None
Assumptions:
- client_socket is a valid socket for reading/writing
*/
static struct response process_query(struct request* query);

#endif // UDPSERV