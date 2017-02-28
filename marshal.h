#include <stdio.h>


typedef struct {
  char name[4096];
  char value[4096];
} Response_header_field;

typedef struct {
  char http_version[50];
  int status_code;
  Response_header_field *fields;
  int field_count;
} Response_header;

typedef struct {
  Response_header header;
  char *body;
} Response;

char* marshal_response(Response *response);