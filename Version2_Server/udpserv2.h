#ifndef UDPSERV2
#define UDPSERV2

#define SERV_HOST_ADDR  "127.0.0.1" // host server address
#define MAX_PACK_LEN 500 // max udp packet size - see udpserver.h for jsutification

/*
Calculate the payload size for a udp packet containing response
Args:
- response: the response payload of the udp packet
Returns:
- the payload size for a udp packet containing response
Assumptions:
- response is a valid response structure
*/
static int calc_packet_size(struct response response);


#endif // UDPSERV