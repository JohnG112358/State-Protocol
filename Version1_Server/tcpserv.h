#ifndef TCPSERV
#define TCPSERV

#include <stdint.h>


#define SERV_HOST_ADDR  "127.0.0.1" // address for the server
#define QUERY_LENGTH  4 // by the specifications for version 1 of the protocol, queries must be 4 bytes
#define HEADER_LENGTH 6 // by version 1 spec, response header must be 6 bytes
#define MAX_DATA_LEN 19000 // max gif length is 19,000 bytes

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
     unsigned char payload[MAX_DATA_LEN];
}__attribute__((packed));

/*
Write a response structure out of a socket
Args:
- client_socket: The socket to write the response structure out of
- res: The response structure to write out
- len: The length of res
Returns:
- None
Assumptions:
- client_socket is a valid socket for writing
- len accurately represents the length of res
*/
static void write_out(int client_socket, struct response res, char* message, int len);

/*
Parses a query to the server and sends the response to the client
Args:
- client_socket: The socket to read/write from
Returns:
- None
Assumptions:
- client_socket is a valid socket for reading/writing
*/
static void process_query(int client_socket);

#endif // TCPSERV