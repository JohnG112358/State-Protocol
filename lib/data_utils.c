/*
Library to save and load the textual/image data needed for the state protocol.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "data_utils.h"

void parseArgs(const int argc, const char* argv[], int* port)
{
    if (argc != 2){
        fprintf(stderr, "The server takes one argument: the port to listen on\n");
        exit(1);
    }

    int tempPort;
    if (1 == sscanf(argv[1], "%d", &tempPort)){
        if (tempPort < 0 || tempPort > 65485){ // ensure we have enough room to test multiple ports in case one is taken
            fprintf(stderr, "Port number must be between 0 and 65485 (to allow trying multiple ports in case one is taken)\n");
            exit(1);
        }
        *port = tempPort;
    }
    else{
        fprintf(stderr, "Port number must be an integer\n");
        exit(1);
    }
}

static char* construct_gif_filename(const char char1, const char char2)
{
    char *filename = (char *)calloc(14, sizeof(char));
    if (filename == NULL) {
        fprintf(stderr, "Couldn't allocate gif filename\n");
        exit(1);
    }

    snprintf(filename, 14, "flags/%c%c.gif", tolower(char1), tolower(char2));

    return filename;
}

static char* construct_local_gif_filename(const char char1, const char char2)
{
    char *filename = (char *)calloc(7, sizeof(char));
    if (filename == NULL) {
        fprintf(stderr, "Couldn't allocate gif filename\n");
        exit(1);
    }

    snprintf(filename, 14, "%c%c.gif", tolower(char1), tolower(char2)); // same as construct_gif_filename but constructs a file path within the current working directory

    return filename;
}


char* no_state_msg(char char1, char char2)
{
    char* message = (char *)calloc(28 + 2 + 1, sizeof(char)); // state codes always 2 digits - we will have checked for this by the time this metod is called
    if (message == NULL) {
        fprintf(stderr, "Couldn't allocate error message\n");
        exit(1);
    }

    snprintf(message, 31, "Error - No such state code %c%c", tolower(char1), tolower(char2));
    
    return message;
}


char* find_data(char char1, char char2, int index)
{
    char state[3];
    sprintf(state, "%c%c", toupper(char1), toupper(char2)); // state codes always 2 digits - we will have checked for this by the time this metod is called

    FILE* file;
    char line[MAX_LINE_LEN];

    file = fopen("statedb.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char* token = strtok(line, "|"); // database entries delimited by pipe character
        if (strcmp(token, state) == 0){
            int count = 0;
            while (count < index){
                token = strtok(NULL, "|");
                count++;
            }
            char* token_str = (char*)calloc(MAX_LINE_LEN, sizeof(char));
            strncpy(token_str, token, MAX_LINE_LEN - 1);

            return token_str;
        }
    }

    return NULL;
}


unsigned char* load_gif(char char1, char char2, long* len)
{
    char* filename = construct_gif_filename(char1, char2);

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error reading GIF file - likely due to state not existing\n");
        return NULL;
    }

    // need to determine the length of the file to read the gif
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    *len = fileSize;

    unsigned char* fileData = (unsigned char*)calloc(fileSize + 1, sizeof(unsigned char));
    if (!fileData) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        exit(1);
    }

    int total = 0;
    for (;;){
        int bytesRead = fread(fileData + total, sizeof(char), fileSize, file);
        total += bytesRead;
        fprintf(stderr, "Bytes read: %d\n", bytesRead);
        if (bytesRead == 0){
            break;
        }
    }

    fprintf(stderr, "Total bytes read: %d\n", total);

    free(filename);
    fclose(file);
    return fileData;
}


void save_gif(const char char1, const char char2, unsigned char* buffer, const int len)
{
    char* filename = construct_local_gif_filename(char1, char2);

    FILE *file = fopen(filename, "wb"); // we write the inary representation of the gif
    if (file == NULL) {
        fprintf(stderr, "Couldn't open file for gif\n");
        exit(1);
    }

    int total = 0;
    for (;;){
        int bytes_written = fwrite(buffer, sizeof(unsigned char), len, file);
        if (bytes_written < 0) {
            fprintf(stderr, "Error writing to file\n");
            fclose(file);
            free(filename);
            exit(1);
        }

        total += bytes_written;
        if (total >= len){
            break;
        }
    }

    fprintf(stderr, "Total bytes read: %d\n", total);

    free(filename);
    fclose(file);
}
