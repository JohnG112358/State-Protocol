#ifndef DATA
#define DATA

#define MAX_LINE_LEN 160 // max length of any line in statedb.txt


/*
Constructs a message that a certain state code doesn't exist
Args:
- char1: first character of the state code
- char2: second character of the state code
Returns:
- A string with the no state code message
Assumptions:
- the caller is responsible for freeing the returned data
*/
char* no_state_msg(char char1, char char2);

/*
Given a state code and an opp code, returns the appropriate data from the database
Args:
- char1: first character of the state code
- char2: second character of the state code
- index: opp code
Returns:
- A string with the requested data
- NULL if the state does not exist
Assumptions:
- index is a valid opp code
- the caller is responsible for freeing the returned data
*/
char* find_data(char char1, char char2, int index);

/*
Loads a state gif into memory as an unsigned char string
Args:
- char1: first character of the state code to load
- char2: second character of the state code to load
- len: pointer to a variable to store the length of the file
Returns:
- A string representing the gif
- NULL if the state code doesn't exist
Assumptions:
- the caller is responsible for freeing the returned data
*/
unsigned char* load_gif(char char1, char char2, long* len);

/*
Saves a gif from memory to a file
Args:
- char1: first character of the state code to save
- char2: second character of the state code to save
- buffer: the in-memory representation of the gif
- len: the length of the gif buffer to save
Returns:
- None
Assumptions:
- len is valid
*/
void save_gif(const char char1, const char char2, unsigned char* buffer, const int len);

/*
Validate arguments for all state protocol servers
Args:
- port: variable to save command argument port to
Returns:
- None
Assumptions:
- None
*/
void parseArgs(const int argc, const char* argv[], int* port);

#endif // DATA
