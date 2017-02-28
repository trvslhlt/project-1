#include <stdlib.h>
#include <string.h>
#include "marshal.h"


#define MAX_FIELD_SIZE 8192
#define DUMB_DEFAULT_SIZE 10000

extern char* get_reason(int status_code);
extern char* marshal_response_header(Response_header *header);
extern char* marshal_response_header_field(Response_header_field *field);
extern char* marshal_status(Response_header *header);


char* marshal_response(Response *response) {
  char *data = malloc(DUMB_DEFAULT_SIZE);
  char *header_data = marshal_response_header(&(response->header));
  strcpy(data, header_data);
  free(header_data);
  strcat(data, "\n");
  if (response->body) {
    strcat(data, response->body);
    strcat(data, "\n");
  }
  return data;
}

char* marshal_response_header(Response_header *header) {
  int i;
  char *marshalled;
  char *buf = (char *) malloc(DUMB_DEFAULT_SIZE);
  char *status_line = marshal_status(header);
  strcpy(buf, status_line);
  free(status_line);
  
  for (i = 0; i < header->field_count; i++) {
    marshalled = marshal_response_header_field(&(header->fields[i]));
    strcat(buf, marshalled);
    free(marshalled);
  }
  return buf;
}

char* marshal_status(Response_header *header) {
  char *buf = (char *)malloc(DUMB_DEFAULT_SIZE);
  char *reason = get_reason(header->status_code);
  char *status[3];
  sprintf(status, "%d", header->status_code);
  
  strcpy(buf, header->http_version);
  strcat(buf, " ");
  strcat(buf, status);
  strcat(buf, " ");
  strcat(buf, reason);
  strcat(buf, "\r\n");
  return buf;
}

char* marshal_response_header_field(Response_header_field *field) {
  char *buf = (char *)malloc(DUMB_DEFAULT_SIZE);
  strcpy(buf, field->name);
  strcat(buf, ": ");
  strcat(buf, field->value);
  strcat(buf, "\r\n");
  return buf;
}

char* get_reason(int status_code) {
  char *reason;
  switch (status_code) {
    case 100:
      reason = "Continue";
      break;
    case 200:
      reason = "OK";
      break;
    case 400:
      reason = "Bad Request";
      break;
    case 404:
      reason = "Not Found";
      break;
    case 411:
      reason = "Length Required";
      break;
    case 500:
      reason = "Internal Server Error";
      break;
    case 501:
      reason = "Not Implemented";
      break;
    case 503:
      reason = "Service Unavailable";
      break;
    case 505:
      reason = "HTTP Version not supported";
      break;
    default:
      reason = "Internal Server Error";
  }
  return reason;
}