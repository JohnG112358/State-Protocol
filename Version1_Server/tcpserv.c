/*
TCP server for version 1 of the state protocol
*/

#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <netinet/in.h>
#include  <arpa/inet.h>
#include  <unistd.h>  
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>
#include <signal.h>   
#include  <stdlib.h>  
#include <stdbool.h>
#include <ctype.h>

#include "tcpserv.h"
#include "data_utils.h"

// print a message whenever a child thread exits
static void child_exit(int signo)
{
    for (;;) {
        int status;
        int pid = waitpid(-1, &status, WNOHANG);
        if (pid == -1)
            break;
        fprintf(stderr, "Child process pid=%d exited\n", pid);
    };
}

static void write_out(int client_socket, struct response res, char* message, int len)
{
    for (int i = 0; i<len; i++){ // copy message into response structure
        res.payload[i] = message[i];
    }

    int n = 0;
    for (;;) { // write until we have written entire structure
        int k = write(client_socket, (void*) &res, sizeof(res));

        if (k<0){
            fprintf(stderr, "Error writing\n");
            exit(1);
        }
        n+=k;

        if (n == sizeof(res)){
            break;
        }
    }
}

static void process_query(int client_socket)
{
    unsigned char queryBuff[QUERY_LENGTH];
    memset(queryBuff, 0, sizeof(queryBuff));

    int n;
    struct response res;
    memset(&res, 0, sizeof(res));

    res.version = 1;
    res.len = htonl(MAX_DATA_LEN);

    for (int i = 0; i<QUERY_LENGTH; i++){ // read in query
        n = recv(client_socket, &queryBuff[i], 1, 0); 
        if (n <= 0){
            fprintf(stderr, "Protocol error: client query was too short\n");
            exit(1);
        }
    }

    struct request* query = (struct request*)queryBuff;
    if (!query){
        fprintf(stderr, "Couldn't allocate query structure, likely due to misallignment\n");
        exit(1);
    }

    if (query->char1 == '\0' || query->char2 == '\0'){
        fprintf(stderr, "Statecode has null characters\n");
        exit(1);
    }

    if (query->version != 1){
        res.status = 255;
        
        char message[] = "Error - Recieved incorrect protocol version";
        write_out(client_socket, res, message, strlen(message));
    }
    if (query->opcode < 1 || query->opcode > 5){
        res.status = 255;

        char* message = (char *)calloc(25 + 3 + 1, sizeof(char)); // 25 for the length of the message, 3 because 1 byte can cover a max of 3 digits
        if (message == NULL) {
            fprintf(stderr, "Couldn't allocate error message\n");
            exit(1);
        }

        snprintf(message, 29, "Error - No such opcode - %d", query->opcode);

        write_out(client_socket, res, message, strlen(message));
    }
    else if (query->opcode != 5){
        char* data = find_data(query->char1, query->char2, query->opcode); // handle opcodes that return textual data

        if (!data){ // find_data returns null if the statecode doesn't exist
            res.status = 255;
            char* message = no_state_msg(query->char1, query->char2);
            write_out(client_socket, res, message, strlen(message));
            free(message);
        }
        else{
            res.status = 1;
            if (query->opcode == 4){
                data[strlen(data) - 1] = '\0'; // ensure response is formatted correctly - file has a '\n' character
            }
            write_out(client_socket, res, data, strlen(data));
            free(data);
        }
    }
    else if (query->opcode == 5){
        long len = 0;
        char* bytes = (char*)load_gif(query->char1, query->char2, &len);

        if (!bytes){ // bytes returns null if the state code doesn't exist
            res.status = 255;
            char* message = no_state_msg(query->char1, query->char2);
            write_out(client_socket, res, message, strlen(message));
            free(message);
        }
        else{
            res.status = 1;
            write_out(client_socket, res, bytes, len);
            free(bytes);
        }
    }
}


int main(int argc, const char* argv[])
{   
    int port;
    parseArgs(argc, argv, &port);

    int sockfd, newsockfd;
    socklen_t cli_len;
    struct sockaddr_in	cli_addr, serv_addr;

    signal(SIGPIPE, SIG_IGN); // prevents a crash if the server is sending data and a client drops the connection
    signal(SIGCHLD, child_exit); // print data when a child thread exits

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Can't open tcp socket\n");
        exit(1);
    }

    // allow the reuse of local addresses for binding, even if the socket is still in a TIME_WAIT state from a previous close operation
    int optval = 1;
    int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    if (err){
        fprintf(stderr, "Error setting socket options for socket accepting incoming connections\n");
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

    err = listen(sockfd, 5);
    if (err) {
        fprintf(stderr, "Error listenting on %s, %d.\n", SERV_HOST_ADDR, port_found);
    }

    // loop forever accepting connections and handling queries
    for ( ; ; ) {
        int pid;
        cli_len = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
        if (newsockfd == -1) {
            fprintf(stderr, "Error accepting connection from client - perhaps the client closed its connection\n");
            continue; 
        }
        else{
            printf("Connection accepted from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        }

        pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Fork error");
            continue;
        }
        else if (pid != 0) {
            // parent process 
            fprintf(stderr, "Spawned child pid=%d\n", pid);
            close(newsockfd);
            continue; // this process continues accepting new connections 
        }
        else if (pid == 0){
            // I am the child process to handle only this connection 
            close(sockfd);
            sockfd = -1;

            fprintf(stderr, "In child pid %d, calling process_query\n", getpid());  
            process_query(newsockfd);

            close(newsockfd);
            break;
        }
    }
}
