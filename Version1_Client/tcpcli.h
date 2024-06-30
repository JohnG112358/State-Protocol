#ifndef TCPCLI
#define TCPCLI

#include <stdint.h>

#define SERV_TCP_PORT   5050 
//#define SERV_HOST_ADDR  "127.0.0.1" - allows us to use out own tcp server
#define SERV_HOST_ADDR  "ip"  // use pre-written server by default for testing purposes
#define HEADER_LENGTH 6 // length of the response header

// incoming request
struct request {
     uint8_t  version;
     uint8_t opcode;
     char char1;
     char char2;
}__attribute__((packed));

// header for the response packet
struct responseHeader {
     uint8_t  version;
     uint8_t  status;
     uint32_t len;
}__attribute__((packed));

/*
Parse arguments for the client
Args:
- opcode: variable to store the desired opcode
- char1: variable to store the first character of the desired state code
- char2: variable to store the second character of the desired state code
Returns:
- None
Assumptions:
- None
*/
static void parse_client_args(const int argc, const char* argv[], int* opcode, char* char1, char* char2);

/*
Sends a request to the server and parses the response
Args:
- client_socket: socket to read from/write to
- req: structure representing request to send to server
Returns:
- None
Assumptions:
- req is a valid request structure
- client_socket is a valid socket to read from/write to
*/
static void send_and_recieve(int client_socket, struct request req);

#endif // TCPCLI