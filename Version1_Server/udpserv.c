/*
UDP server for version 1 of the state protocol
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

#include "udpserv.h"
#include "data_utils.h"

// copy a message into a response structure
static void copy(struct response *res, char* message, int len)
{
    for (int i = 0; i<len; i++){
        res->payload[i] = message[i];
    }
}

static struct response process_query(struct request* query)
{
    struct response res;
    memset(&res, 0, sizeof(res));

    res.version = 1;
    res.len = htonl(MAX_PACK_LEN);

    if (query->version != 1){
        res.status = 255;
        fprintf(stderr, "Recieved incorrect protocol version %d\n", query->version);
        char message[] = "Error - Recieved incorrect protocol version";
        copy(&res, message, strlen(message));
    }
    else if (query->opcode < 1 || query->opcode > 5){
        res.status = 255;

        char* message = (char *)calloc(25 + 3 + 1, sizeof(char)); // 25 for the length of the message, 3 because 1 byte can cover a max of 3 digits
        if (message == NULL) {
            fprintf(stderr, "Couldn't allocate error message\n");
            exit(1);
        }
        fprintf(stderr, "Recieved non-existant opcode %d\n", query->opcode);
        snprintf(message, 29, "Error - No such opcode - %d", query->opcode);
        copy(&res, message, strlen(message));
    }
    else if(query->opcode == 5){ // all gifs are too large for udp - we don't need to bother checking the size
        res.status = 254;
        fprintf(stderr, "Recieved request for state flag, but this is too large to send over udp\n");
        char message[] = "All state flags are too large for udp - try tcp";
        copy(&res, message, strlen(message));
    }
    else if(query->char2 != 5){
        char* data = find_data(query->char1, query->char2, query->opcode); // no individual data entry is longer than 500 bytes so it is guaranteed to fit in the packet buffer

        if (!data){
            res.status = 255;
            char* message = no_state_msg(query->char1, query->char2);
            copy(&res, message, strlen(message));
            free(message);
        }
        else{
            res.status = 1;
            if (query->opcode == 4){
                data[strlen(data) - 1] = '\0'; // remove newline that's inserted at the end of each database entry
            }
            copy(&res, data, strlen(data));
            free(data);
        }
    }

    return res;
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

    // loop forever accepting connections and handling queries
    for (;;){
        fprintf(stderr, "Waiting for connections.\n");
        cli_len = sizeof(cli_addr);

        int n;
        unsigned char queryBuff[QUERY_LENGTH];
        memset(queryBuff, 0, sizeof(queryBuff));
        
        // read udp datagram (whatever we got from the client)
        n = recvfrom(sockfd, &queryBuff, QUERY_LENGTH, 0, (struct sockaddr*)&cli_addr, &cli_len);
        fprintf(stderr, "Read %d bytes\n", n);
        
        if (n < 0){
            fprintf(stderr, "Error reading from socket\n");
            continue;
        }
        if (n < 4){
            fprintf(stderr, "Protocol error: client query was too short.  As this is udp, we discard the message\n");
            continue;
        }
        if (n > 4){
            fprintf(stderr, "Protocol error: client query was too long.  As this is udp, we discard the message\n");
            continue;
        }
        
        fprintf(stderr, "Successfully recieved request from %s\n", inet_ntoa(cli_addr.sin_addr));

        struct request* query = (struct request*)queryBuff;
        if (!query){
            fprintf(stderr, "Couldn't allocate query structure, likely due to misallignment\n");
            exit(1);
        }
        
        if (query->char1 == '\0' || query->char2 == '\0'){
            fprintf(stderr, "Statecode has null characters\n");
            continue;
        }

        fprintf(stderr, "Calling process query\n");
        struct response res = process_query(query);

        fprintf(stderr, "Sending %s\n", res.payload);
        // write out data in a single datagram
        n = sendto(sockfd, &res, sizeof(struct response), 0, (struct sockaddr *) &cli_addr, cli_len);
        fprintf(stderr, "Sent %d bytes\n", n);

        if (n < 0){
            fprintf(stderr, "Error sending response to client\n"); // this is udp so we "fire and forget"
            continue;
        }
    }
}
