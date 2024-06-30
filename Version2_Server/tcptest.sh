#!/bin/bash

# to test, you first need to launch the server on the pond running on port 5050

# null character bytes 
echo -ne '\x02\x02\x00h\x03\x03' | nc 127.0.0.1 5050 | xxd
echo -ne '\x02\x02n\x00\x03\x03' | nc 127.0.0.1 5050 | xxd

# wrong version
echo -ne '\x03\x02ma\x03\x03' | nc 127.0.0.1 5050 | xxd

# negative num opcodes
echo -ne '\x02\xffnh\x03\x03' | nc 127.0.0.1 5050 | xxd

# opcodes field too large
echo -ne '\x02\x07nh\x03\x03' | nc 127.0.0.1 5050 | xxd

# invalid positive opcode
echo -ne '\x02\x02nh\x03\x07' | nc 127.0.0.1 5050 | xxd

# invalid negative opcode
echo -ne '\x02\x02nh\xff\x03' | nc 127.0.0.1 5050 | xxd

# invlaid state for text retrieval 
echo -ne '\x02\x02tt\x03\x03' | nc 127.0.0.1 5050 | xxd

# invalid state for gif retrieval 
echo -ne '\x02\x01tt\x05' | nc 127.0.0.1 5050 | xxd

# repeated text opcode too many times
echo -ne '\x02\x05nh\x01\x01\x01\x01\x01' | nc 127.0.0.1 5050 | xxd

# valid query - single text opcode
echo -ne '\x02\x01nh\x02' | nc 127.0.0.1 5050 | xxd

# valid query - single gif opcode
echo -ne '\x02\x01nh\x05' | nc 127.0.0.1 5050 | xxd

# valid query - all five opcodes
echo -ne '\x02\x05nh\x01\x02\x03\x04\x05' | nc 127.0.0.1 5050 | xxd

# valid query - repeated text opcode
echo -ne '\x02\x04nh\x01\x01\x01\x01' | nc 127.0.0.1 5050 | xxd

# valid query - repeated gif opcode
echo -ne '\x02\x05nh\x05\x05\x05\x05\x05' | nc 127.0.0.1 5050 | xxd