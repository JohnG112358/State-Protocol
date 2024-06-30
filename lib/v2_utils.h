#ifndef V2
#define V2

#include <stdint.h>

#define MAX_OPCODES 5 // asuming 5 opcodes max
#define MAX_DATA_LEN 120 // maximum length of textual data
#define MAX_GIF_LEN 19000 // maximum gif length
#define QUERY_LEN 9 // each state has max 5 opcodes, so max query length is 9
#define HEADER_LEN 4 // response header length is 4

// an individual response to a single opcode in the overall response
struct payload {
    uint32_t len;
    unsigned char payload[MAX_DATA_LEN];
}__attribute__((packed));

// the response to a gif query
struct gif_payload{
    uint32_t len;
    unsigned char payload[MAX_GIF_LEN];
}__attribute__((packed));

// a representation of imcoming requests
struct request{
    uint8_t  version;
    uint8_t  num_queries;
    char char1;
    char char2;
    unsigned char opcodes[MAX_OPCODES];
}__attribute__((packed));

// a representation of the response packet
struct response{
    uint8_t  version;
    uint8_t  status;
    uint8_t  num_responses;
    uint8_t  reserved;
    struct payload payloads[4]; 
    struct gif_payload gif; 
}__attribute__((packed));

/*
Parse a version 2 query and either reject it or cast it to a query structure
Args:
- queryBuff: the raw bytes we read in over the network
Returns:
- NULL if the request is invalid
- a request query if the client request is valid
Assumptions:
- queryBuff is exactly 9 bytes long, with bytes that the client didn't send set to null
*/
struct request* parse_query(unsigned char queryBuff[]);

/*
Executes a version 2 query and returns a response structure with the result
Args:
- req: The request structure generated from parsing a query
Returns:
- A response structure with the results of the query (this is an error response if the query was invalid)
Assumptions:
- None
*/
struct response execute_query(struct request* req);

/*
Prints a response structure - useful for logging what the server is sending
Args:
- response: The response structure we want to print
Returns:
- None
Assumptions:
- response is a valid response structure
*/
void print_response(struct response response);

/*
Creates a response to convey than an error has occured
Args:
- None
Returns:
- None
Assumptions:
- None
*/
struct response error_response();

/*
Creates a response to convey than the query will generate a response that is too large for udp
Args:
- None
Returns:
- None
Assumptions:
- None
*/
struct response udp_len_response();

#endif // V2