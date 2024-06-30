/*
TCP server for version 2 of the state protocol
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

#include "data_utils.h"
#include "v2_utils.h"
#include "tcpserv2.h"

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

// write header over tcp
static void write_header(int client_socket, struct response res)
{
    int n = 0;
    for (;;) { // write until all the data has been written
        int k = write(client_socket, (void*) &res, HEADER_LEN);

        if (k<0){
            fprintf(stderr, "Error writing header\n");
            perror("write");
            return;
        }
        n+=k;

        if (n == HEADER_LEN){
            break;
        }
    }
}

// write text payload over tcp
static void write_payload(int client_socket, struct payload payload)
{
    int n = 0;
    if (payload.len < 1){
        return;
    }

    for (;;) {
        int k = write(client_socket, (void*) &payload, sizeof(payload));

        if (k<0){
            fprintf(stderr, "Error writing payload\n");
            perror("write");
            return;
        }
        n+=k;
        

        if (n == sizeof(payload)){
            break;
        }
    }
}

// write gif payload over tcp
static void write_gif(int client_socket, struct gif_payload payload)
{
    if (payload.len < 1){
        return;
    }

    int n = 0;

    for (;;) {
        int k = write(client_socket, (void*) &payload, sizeof(payload));

        if (k<0){
            fprintf(stderr, "Error writing gif\n");
            perror("write");
            return;
        }
        n+=k;

        if (n == sizeof(payload)){
            break;
        }
    }
}

static void write_out(int client_socket, struct response res)
{
    write_header(client_socket, res);
    for (int i = 0; i < 4; i++){
        write_payload(client_socket, res.payloads[i]);
    }
    write_gif(client_socket, res.gif);
}

static void process_query(int client_socket)
{
    unsigned char queryBuff[QUERY_LEN];
    memset(queryBuff, 0, sizeof(queryBuff));
    int n;

    // read version
    n = recv(client_socket, &queryBuff[0], 1, 0); 
    if (n < 0){
        fprintf(stderr, "Protocol error: error while reading query\n");
        exit(1);
    }

    // read num queries
    // need to get num queries in advance to avoid blocking forever
    n = recv(client_socket, &queryBuff[1], 1, 0); 
    if (n < 0){
        fprintf(stderr, "Protocol error: error while reading query\n");
        exit(1);
    }
    if (n == 0){
        fprintf(stderr, "Client didn't send number opcodes - connection is no longer open so we silently drop this packet\n");
        return;
    }
    if (queryBuff[1] > 5){ // see readme for version 2 implementation assumptions
        fprintf(stderr, "Number of queries must be in the range [0, 5]\n");
        struct response error = error_response();
        write_out(client_socket, error);
        return;
    }

    // read state code and op codes
    for (int i = 2; i<4 + queryBuff[1]; i++){
        n = recv(client_socket, &queryBuff[i], 1, 0); 
        if (n < 0){
            fprintf(stderr, "Protocol error: error while reading query\n");
            exit(1);
        }
        if (n == 0){
            break;
        }
    }

    // print the bytes of the quiery we've read
    fprintf(stderr, "Query: ");
    for (int i = 0; i<9; i++){
        fprintf(stderr, "%d ", queryBuff[i]);
    }
    fprintf(stderr, "\n");


    fprintf(stderr, "Parsing query\n");
    struct request* req = parse_query(queryBuff);
    fprintf(stderr, "Processing query\n");
    struct response res = execute_query(req);
    print_response(res); // log all responses
    write_out(client_socket, res);
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
        fprintf(stderr, "Can't open stream socket\n");
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

            fprintf(stderr, "In child pid %d, calling process_query\n", getpid());           
            process_query(newsockfd);

            fprintf(stderr, "In child pid %d, closing connection\n", getpid());  
            close(newsockfd);
            break;
        }
    }
}