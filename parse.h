#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0

typedef struct {
	char name[4096];
	char value[4096];
} Request_header_field;

typedef struct {
	char http_version[50];
	char method[50];
	char uri[4096];
	Request_header_field *fields;
	int field_count;
} Request_header;

typedef struct {
	Request_header header;
	char *body;
} Request;

int parse(char *buffer, int size, Request *request);
int invalid_request_data(char *request_data); // not asking if complete, only if already invalid or malformed
int complete_request(char *request_data);
