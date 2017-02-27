#include <stdio.h>

//Header field
typedef struct {
	char name[4096];
	char value[4096];
} Response_header_field;

//HTTP Response Header
typedef struct {
	char http_version[50];
	char http_method[50];
	char http_uri[4096];
	Response_header_field *headers;
	int header_count;
} Response_header;

int http_handle_data(char *, int, int, char *);