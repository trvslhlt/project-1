#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0

//Header field
typedef struct {
	char header_name[4096];
	char header_value[4096];
} Request_header;

//HTTP Request Header
typedef struct {
	char http_version[50];
	char http_method[50];
	char http_uri[4096];
	Request_header *headers;
	int header_count;
} Request;

// TODO: might want to change the signature
// so the function can return an error code, and assign a new Request
// struct internally. right now if a null pointer were returned
// it could mean the data in buffer is a partial request, OR
// a complete malformed one
Request* parse(char *buffer, int size,int socketFd);
