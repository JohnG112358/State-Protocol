/*
UDP server for version 2 of the state protocol
*/

#include <stdio.h> 
#include <strings.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_utils.h"
#include "v2_utils.h"
#include "udpserv2.h"

static int calc_packet_size(struct response response)
{
    int size = HEADER_LEN;

    for (int i = 0; i<4; i++){
        if (response.payloads[i].len<1){ // only want to worry about the size of non-null payloads
            continue;
        }
        size += sizeof(struct payload);
    }
    if (response.gif.len > 0){
        size += sizeof(struct gif_payload);
    }

    return size;
}

int main(int argc, const char* argv[]){
    int port;
    parseArgs(argc, argv, &port);

    int sockfd;
    unsigned int cli_len; 
    struct sockaddr_in serv_addr, cli_addr; 

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        fprintf(stderr, "Can't open udp socket\n");
        exit(1);
    }
    fprintf(stderr, "Successfuly opened socket\n");

    // allow the reuse of local addresses for binding, even if the socket is still in a TIME_WAIT state from a previous close operation
    int optval = 1;
    int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    if (err){
        fprintf(stderr, "Error setting socket options\n");
        close(sockfd);
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); 

    // find a port that isn't being used between the port we're given on the command line and 50 above it
    int port_found = 0;
    for (int p = port; p < port + 50; ++p) {
        serv_addr.sin_port = htons(p);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == 0) {
            fprintf(stderr, "Successfully bound to port %d\n", p);
            port_found = p;
            break;
        }
        else {
            fprintf(stderr, "Failed to bind to port %d\n", p);
        }
    }

    if (!port_found) {
        fprintf(stderr, "No available ports in the range.\n");
        close(sockfd);
        exit(1);
    }

    for (;;){
        fprintf(stderr, "Waiting for connections.\n");
        cli_len = sizeof(cli_addr);

        int n;
        unsigned char queryBuff[QUERY_LEN];
        memset(queryBuff, 0, sizeof(queryBuff));
        
        // read udp datagram (whatever we got from the client)
        n = recvfrom(sockfd, &queryBuff, QUERY_LEN, 0, (struct sockaddr*)&cli_addr, &cli_len);
        fprintf(stderr, "Read %d bytes\n", n);
        if (n < 0){
            fprintf(stderr, "Error reading from socket\n");
            continue;
        }
        if (n < HEADER_LEN){
            fprintf(stderr, "Protocol error: client query was too short.\n");
            struct response resp = error_response();
            n = sendto(sockfd, &resp, calc_packet_size(resp), 0, (struct sockaddr *) &cli_addr, cli_len); // only want to send a single udp packet - what gets sent gets sent
            fprintf(stderr, "Sent %d bytes\n", n);
            if (n < 0){
                fprintf(stderr, "Error sending response to client\n");
            }
            continue;
        }
        
        
        fprintf(stderr, "Successfully recieved request from %s\n", inet_ntoa(cli_addr.sin_addr));

        fprintf(stderr, "Parsing query\n");
        struct request* req = parse_query(queryBuff);
        fprintf(stderr, "Processing query\n");
        struct response res = execute_query(req);

        int size = calc_packet_size(res);
        if (size > MAX_PACK_LEN){ // want to respond to client query in a single udp packet
            fprintf(stderr, "Desired response is too large for a single UDP datagram\n");
            struct response error = udp_len_response();
            n = sendto(sockfd, &error, calc_packet_size(error), 0, (struct sockaddr *) &cli_addr, cli_len); // only want to send a single udp packet - what gets sent gets sent
            fprintf(stderr, "Sent %d bytes\n", n);
            if (n < 0){
                fprintf(stderr, "Error sending response to client\n");
            }
            print_response(error); // log all responses
        }
        else{
            print_response(res); // log all responses
            n = sendto(sockfd, &res, size, 0, (struct sockaddr *) &cli_addr, cli_len); // send response to client query
            fprintf(stderr, "Sent %d bytes\n", n);

            if (n < 0){
                fprintf(stderr, "Error sending response to client\n");
            }
        }
    }
}