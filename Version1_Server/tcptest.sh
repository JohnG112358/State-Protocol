#!/bin/bash

# to test, you first need to launch the server on the pond running on port 5050

# null statecode characters
echo -ne '\x01\x03\x00n' | nc 127.0.0.1 5050 | xxd
echo -ne '\x01\x03h\x00' | nc 127.0.0.1 5050 | xxd

# query is too short
echo -ne '\x01' | nc 127.0.0.1 5050 | xxd

# query is too long
echo -ne '\x01\x01\x01nh' | nc 127.0.0.1 5050 | xxd

# wrong version
echo -ne '\x02\x01nh' | nc 127.0.0.1 5050 | xxd

# invalid positive opcode
echo -ne '\x01\x09nh' | nc 127.0.0.1 5050 | xxd

#invalid negative opcode
echo -ne '\x01\xffnh' | nc 127.0.0.1 5050 | xxd

# invalid state
echo -ne '\x01\x01tt' | nc 127.0.0.1 5050 | xxd

# valid opcodes
echo -ne '\x01\x01nh' | nc 127.0.0.1 5050 | xxd
echo -ne '\x01\x02nh' | nc 127.0.0.1 5050 | xxd
echo -ne '\x01\x03nh' | nc 127.0.0.1 5050 | xxd
echo -ne '\x01\x04nh' | nc 127.0.0.1 5050 | xxd
echo -ne '\x01\x05nh' | nc 127.0.0.1 5050 | xxd