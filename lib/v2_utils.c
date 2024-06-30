/*
Library to parse version 2 of the state protocol - used in the tcp and udp version 2 servers
*/

#include <stdint.h>
#include  <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "v2_utils.h"
#include "data_utils.h"

struct request* parse_query(unsigned char queryBuff[])
{
    uint8_t version = queryBuff[0];
    if (version != 2){
        fprintf(stderr, "Version is not version 2\n");
        return NULL;
    }
    
    uint8_t num_queries = queryBuff[1];
    if (num_queries < 1 || num_queries > 5){ // assumung we only want 5 queries - see readme 
        fprintf(stderr, "Num queries must be between 1 and 5.  Length field said %d\n", num_queries);
        return NULL;
    }

    int actual = 0;
    int text_field = 0;
    for (int i = 4; i < QUERY_LEN; i++){
        if (queryBuff[i] != '\0'){
            actual++; // count the actual number of opcodes in the packet
            if (queryBuff[i] < 1 || queryBuff[i] > 5){
                fprintf(stderr, "Recieved invalid opcode %d\n", queryBuff[i]);
                return NULL;
            }
            else if (queryBuff[i] != 5){
                text_field++;
            }
        }
    }

    if (num_queries != actual){
        fprintf(stderr, "Num queries %d did not match actual number of opcodes %d\n", num_queries, actual);
        return NULL;
    }
    if (actual == 0){
        fprintf(stderr, "No opcodes provided\n");
        return NULL;
    }

    if (text_field > 4){ // assumung we only want 4 queries for textual information - see readme 
        fprintf(stderr, "Protocol supports a maximum of 4 text fields\n");
        return NULL;
    }

    if (!queryBuff[2]|| !queryBuff[3]){
        fprintf(stderr, "Statecode had null characters\n");
        return NULL;
    }

    struct request* in = (struct request*)queryBuff; // need to return a pointer vs the structure so this cast is valid

    return in; // we will never need to free in because queryBuff is always a fixed size array allocated by the compiler
}

static struct payload handle_opp(int code, char char1, char char2)
{
    struct payload ret;
    memset(&ret, 0, sizeof(ret));

    char* data = find_data(char1, char2, code);
    if (!data){ // data returns null if the statecode doesn't exist
        fprintf(stderr, "Statecode doesn't exist\n");
        return ret;
    }

    ret.len = htonl(MAX_DATA_LEN);

    for (int i = 0; i<MAX_DATA_LEN; i++){
        if (i<strlen(data)){
            ret.payload[i] = data[i];
        }
        else{
            break;
        }
    }

    return ret;
}

static struct gif_payload handle_gif_opp(char char1, char char2)
{
    struct gif_payload gif;
    memset(&gif, 0, sizeof(gif));

    long len = 0;
    unsigned char* data = load_gif(char1, char2, &len);
    if (!data){
        fprintf(stderr, "Statecode doesn't exist\n");
        return gif;
    }

    gif.len = htonl(MAX_GIF_LEN);
    for (int i = 0; i<MAX_GIF_LEN; i++){
        if (i<len){
            gif.payload[i] = data[i];
        }
        else{
            break;
        }
    }

    return gif;
}

struct response error_response()
{
    struct response res;
    memset(&res, 0, sizeof(res));
    
    res.version = 2;
    res.status = 255;
    res.num_responses = 1;
    res.reserved = 0;

    char error[] = "Invalid query.  Please review the protocol and valid options and try again."; // per lab spec, can have one error message for all errors

    struct payload error_payload;
    memset(&error_payload, 0, sizeof(error_payload));

    error_payload.len=htonl(MAX_DATA_LEN);

    for (int i = 0; i<strlen(error); i++){
        error_payload.payload[i] = error[i];
    }

    res.payloads[0] = error_payload;

    return res;
}

struct response udp_len_response()
{
    struct response res;
    memset(&res, 0, sizeof(res));
    
    res.version = 2;
    res.status = 254;
    res.num_responses = 1;
    res.reserved = 0;

    char error[] = "Response is too large for UDP - try TCP";

    struct payload error_payload;
    memset(&error_payload, 0, sizeof(error_payload));

    error_payload.len=htonl(MAX_DATA_LEN);

    for (int i = 0; i<strlen(error); i++){
        error_payload.payload[i] = error[i];
    }
    res.payloads[0] = error_payload;

    return res;
}

struct response execute_query(struct request* req)
{
    struct response res;
    memset(&res, 0, sizeof(res));

    if (!req){ // something was wrong with the query we recieved
        return error_response();
    }

    struct payload payloads[4];
    struct gif_payload gif;
    memset(payloads, 0, sizeof(payloads));
    memset(&gif, 0, sizeof(gif));

    int sent_gif = 0;
    int curr_payload = 0;
    for (int i = 0; i < req->num_queries; i++){
        if (req->opcodes[i] == 5){
            gif = handle_gif_opp(req->char1, req->char2);
            if(gif.len < 1){
                return error_response(); // error handling the gif request
            }
            sent_gif = 1;
        }
        else{
            struct payload attempt = handle_opp(req->opcodes[i], req->char1, req->char2);
            if(attempt.len < 1){ 
                return error_response(); // error handling the text request
            }
            payloads[curr_payload] = attempt;
            curr_payload++; // we want all payloads adjacent in memory so the packet doesn't have null bytes in the middle
        }
    }

    res.version = 2;
    res.status = 1;
    res.num_responses = curr_payload + sent_gif;
    res.reserved = 0;
    res.gif = gif;
    for (int i = 0; i<4; i++){
        res.payloads[i] = payloads[i];
    }

    return res;
}

static void print_payload(struct payload payload)
{
    if (payload.len < 1){
        return;
    }

    fprintf(stderr, "Payload:\n");
    fprintf(stderr, "- Payload length: %d\n- ", ntohl(payload.len));
    for (int i = 0; i<MAX_DATA_LEN; i++){
        if (payload.payload[i] != '\0'){
            fprintf(stderr, "%c", payload.payload[i]);
        }
    }
    fprintf(stderr, "\n");
}

static void print_gif(struct gif_payload payload)
{
    if (payload.len < 1){
        return;
    }

    fprintf(stderr, "gif:\n");
    fprintf(stderr, "- Payload length: %d\n", ntohl(payload.len));
    fprintf(stderr, "- Saved gif locally\n");
    save_gif('t', 't', payload.payload, MAX_GIF_LEN); // can't print gif so we save it
}

void print_response(struct response response)
{
    fprintf(stderr, "Response:\n");
    fprintf(stderr, "- Version: %d\n", response.version);
    fprintf(stderr, "- Status: %d\n", response.status);
    fprintf(stderr, "- Num Responses: %d\n", response.num_responses);
    fprintf(stderr, "- Reserved: %d\n", response.reserved);
    for (int i = 0; i<4; i++){
        print_payload(response.payloads[i]);
    }
    print_gif(response.gif);
}


// unit tests - we also unit test the data_utils library because all of those functions are called by this library
#ifdef TEST
int main() {
    struct request* ans;
    struct response packet;

    // wrong version
    unsigned char byteArray1[] = { 3, 3, 'n', 'h', 1, 5, '\0', '\0', '\0'};
    ans = parse_query(byteArray1);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // negative num opcodes
    unsigned char byteArray2[] = { 2, -1, 'n', 'h', 1, 5, '\0', '\0', '\0'};
    ans = parse_query(byteArray2);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // opcodes field too large
    unsigned char byteArray3[] = { 2, 7, 'n', 'h', 1, 2, 3, '\0', '\0'};
    ans = parse_query(byteArray3);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // num opcodes doesn't match actual number of opcodes
    unsigned char byteArray4[] = { 2, 4, 'n', 'h', 1, 2, 3, '\0', '\0'};
    ans = parse_query(byteArray4);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    unsigned char byteArray5[] = { 2, 4, 'n', 'h', 1, 2, 3, 4, 4, 4, 4, 4};
    ans = parse_query(byteArray5);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // invalid positive opcode
    unsigned char byteArray6[] = { 2, 3, 'n', 'h', 1, 7, 3, '\0', '\0'};
    ans = parse_query(byteArray6);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // invalid negative opcode
    unsigned char byteArray7[] = { 2, 3, 'n', 'h', 1, -1, 3, '\0', '\0'};
    ans = parse_query(byteArray7);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // null character bytes 
    unsigned char byteArray8[] = { 2, 3, '\0', 'h', 1, 2, 3, '\0', '\0'};
    ans = parse_query(byteArray8);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    unsigned char byteArray9[] = { 2, 3, 'n', '\0', 1, 2, 3, '\0', '\0'};
    ans = parse_query(byteArray9);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // invlaid state for text retrieval 
    unsigned char byteArray10[] = { 2, 1, 'm', 'm', 1, '\0', '\0', '\0', '\0'};
    ans = parse_query(byteArray10);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // invalid state for gif retrieval 
    unsigned char byteArray11[] = { 2, 1, 'm', 'm', 5, '\0', '\0', '\0', '\0'};
    ans = parse_query(byteArray11);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // valid query - single text opcode
    unsigned char byteArray12[] = { 2, 1, 'n', 'h', 5, '\0', '\0', '\0', '\0'};
    ans = parse_query(byteArray12);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // valid query - single gif opcode
    unsigned char byteArray13[] = { 2, 1, 'n', 'h', 1, '\0', '\0', '\0', '\0'};
    ans = parse_query(byteArray13);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // valid query - all five opcodes
    unsigned char byteArray14[] = { 2, 5, 'n', 'h', 1, 2, 3, 4, 5 };
    ans = parse_query(byteArray14);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // valid query - repeated text opcode
    unsigned char byteArray15[] = { 2, 5, 'n', 'h', 1, 1, 1, 1, 1 };
    ans = parse_query(byteArray15);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");

    // valid query - repeated gif opcode
    unsigned char byteArray16[] = { 2, 5, 'n', 'h', 5, 5, 5, 5, 5 };
    ans = parse_query(byteArray16);
    packet = execute_query(ans);
    print_response(packet);
    fprintf(stderr, "\n");
}
#endif