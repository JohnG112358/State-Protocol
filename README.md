# Custom State Protocol Implementation

This project contains my implementation of TCP and UDP clients/servers for two versions of a protocol to send information about different US states.  Detailed documentation for the protocol and all functions is included in the .h files.\
\
Note: Sometimes netcat can be finnicky when running the test scripts by leaving a hanging connection, preventing the script from finishing.  However, if you run each command individually, they will all pass. 

## File Structure

### Lib

A directory containing libraries I used in other parts of this lab.  

Files:
- data_utils.c: A library for handling reading/writing state data (e.g., gifs, text, etc.)
- v2_utils.c: A library to parse version 2 of the state protocol

Makefile Targets:
- all: Makes the .o files for both libraries
- test: Makes executables contaning unit tests for the libraries
- clean: Removes executables, .o files, etc.

### Version 1 Client

A directory containing a TCP client for version 1 of the state protocol

Files:
- tcpcli.c: The tcp client for state protocol version 1
- test.sh: Test cases for the client

Makefile Targets:
- all: Makes an executable version of the client
- ec: Makes the extra credit version of the client that reads the port and server address from environment variables
- test: Runs the testing script for the client
- clean: Removes executables, .o files, etc.

### Version 1 Server

A directory containing the TCP and UDP servers for version 1 of the state protocol

Files:
- tcpserv.c: The TCP server for version 1
- udpserv.c: The UDP server for version 1
- tcptest.sh: Test cases for the TCP server
- udptest.sh: Test cases for the UDP server

Makefile Targets:
- all: Makes executables for the TCP server and the UDP server
- clean: Removes executables, .o files, etc.

### Version 2 Server

A directory containing the TCP and UDP servers for version 2 of the state protocol

Files:
- tcpserv2.c: The TCP server for version 2
- udpserv2.c: The UDP server for version 2
- tcptest.sh: Test cases for the TCP server
- udptest.sh: Test cases for the UDP server

Makefile Targets:
- all: Makes executables for the TCP server and the UDP server
- clean: Removes executables, .o files, etc.
