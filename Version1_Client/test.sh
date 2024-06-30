#!/bin/bash

# valid opcodes
./client nh 1
./client nh 2
./client nh 3
./client nh 4
./client nh 5

# opcode too large
./client nh 8

# opcode too small
./client nh 0

# state code doesn't exist
./client tt 0

# state code is too short
./client t 0

# state code is too long
./client ttt 0