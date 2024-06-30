/*
TCP client for state protocol version 1
*/

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "tcpcli.h"
#include "data_utils.h"


static void parse_client_args(const int argc, const char* argv[], int* opcode, char* char1, char* char2)
{
    if (argc != 3){
        fprintf(stderr, "The client requires exactly three arguments\n");
        exit(1);
    }
    
    char* tempState = malloc(100 * sizeof(char));
    if (1 == sscanf(argv[1], "%s", tempState)){ // check first arg is a valid string
        if (strlen(tempState) == 2){
            *char1 = tempState[0];
            *char2 = tempState[1];
        }
        else{
            fprintf(stderr, "The state abbreviation must have exactly two letters\n");
            exit(1);
        }
    }
    else{
        fprintf(stderr, "The first argument must be the two letter abbreviation of a state\n");
        exit(1);
    }

    int tempCode;
    if (1 == sscanf(argv[2], "%d", &tempCode)){
        if (tempCode < 0 || tempCode > 6){
            fprintf(stderr, "Opcode must be in the range [1, 5]\n"); // prevents overflow issues for the uint8
            exit(1);
        }
        *opcode = tempCode;
    }
    else{
        fprintf(stderr, "Opcode must be an integer\n");
        exit(1);
    }
}

static void send_and_recieve(int client_socket, struct request req)
{
    int n = 0;
    for (;;) { // repeat until entire request has been written
        n += write(client_socket, (void*) &req, sizeof(req));
        if (n == sizeof(req)){
            break;
        }
    }

    unsigned char headerBuff[HEADER_LENGTH];

    // we are guaranteed to recieve one byte from the recv menthod assuming there is no error
    for (int i = 0; i< HEADER_LENGTH; i++){
        n = recv(client_socket, &headerBuff[i], 1, 0); 
        if (n <= 0){
            fprintf(stderr, "Recieved server response whose header was too short\n");
            exit(1);
        }
    }

    struct responseHeader *header = (struct responseHeader *)headerBuff;
    if (header == 0){
        fprintf(stderr, "Couldn't allocate response structure\n");
        exit(1);
    }

    uint8_t version = header->version;
    uint8_t status = header->status;
    uint32_t len = ntohl(header->len);
    
    if (version != 1){
        fprintf(stderr, "Recieved wrong protocol version\n");
        exit(1);
    }

    unsigned char *payloadBuff = calloc(len + 1, sizeof(char));
    int total = 0;
    for (;;){ // loop until we have recieved the sevrer's entire response
        int count = recv(client_socket, payloadBuff + total, len, 0);
        total += count;
        if (count < 0) {
            fprintf(stderr, "Error with recieve call\n");
            exit(1);
        }
        if (total == len){
            break;
        }
        else if (count == 0  && total<len){
            fprintf(stderr, "Payload was too short - expected %d got %d\n", len, total);
            exit(1);
        }
        else if (total > len){
            fprintf(stderr, "Payload was too long - expected %d got %d\n", len, total);
            exit(1);
        }
    }

    if (status == 255){ // status code 255 is always an error
        printf("Recieved error: %s\n", payloadBuff);
    }
    else if (status == 1 && req.opcode == 5){
        printf("Saved state flag locally\n");
        save_gif(req.char1, req.char2, payloadBuff, len);
    }
    else if (status == 1){
        printf("%s\n", payloadBuff);
    }
}

int main(int argc, const char* argv[])
{   
    int client_fd;
    struct sockaddr_in serv_addr;
    struct request req;

    int opcode;
    char char1;
    char char2;

    parse_client_args(argc, argv, &opcode, &char1, &char2);

    char address_var[] = "STATE_INFO_HOST";
    char port_var[] = "STATE_INFO_PORT";

    char* addr = getenv(address_var);
    char* port_str = getenv(port_var);

    if (!addr){
        fprintf(stderr, "Must define an address via the STATE_INFO_HOST environment variable\n");
        exit(1);
    }
    if (!port_str){
        fprintf(stderr, "Must define a port via the STATE_INFO_PORT environment variable\n");
        exit(1);
    }

    int port = atoi(port_str);
    if (port < 0 || port > 65535){
        fprintf(stderr, "Port must be in the range [0, 65535]\n");
        exit(1);
    }
    
    req.version = 1;
    req.opcode = opcode;
    req.char1 = char1;
    req.char2 = char2;

    // open socket to communicate with server
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation error \n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // make sure server address is valid
    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address\n");
        exit(1);
    }

    // connect to server
    if ((connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        fprintf(stderr, "Connection Failed \n");
        exit(1);
    }

    send_and_recieve(client_fd, req);
    
    // clean up
    close(client_fd);
    return 0;
}