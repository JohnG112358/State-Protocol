#ifndef TCPSERV2
#define TCPSERV2

#define SERV_HOST_ADDR  "127.0.0.1" // host address for the server

/*
Write a response structure out of a socket
Args:
- client_socket: The socket to write the response structure out of
- res: The response structure to write out
Returns:
- None
Assumptions:
- client_socket is a valid socket for writing
*/
static void write_out(int client_socket, struct response res);

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