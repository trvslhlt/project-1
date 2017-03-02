#include <stdlib.h>
#include <string.h>
#include "marshal.h"


#define MAX_FIELD_SIZE 8192
#define BIG_DUMB_NUMBER 10000

extern void marshal_response_header(Response_header *header, char *buf);
extern void marshal_response_header_field(Response_header_field *field, char *buf);
extern void marshal_status(Response_header *header, char *buf);


void marshal_response(Response *response, char* data) {
  printf("mmmmmmmmm marshal_response\n");
  char header_data[BIG_DUMB_NUMBER];
  marshal_response_header(&(response->header), header_data);
  strcpy(data, header_data);
  if (response->body) {
    printf("mmmmmmmmm has body %s\n", response->body);
    strcat(data, "\n");
    strcat(data, response->body);
  }
  strcat(data, "\r\n");
}

void marshal_response_header(Response_header *header, char *buf) {
  printf("mmmmmmmmm marshal_response_header\n");
  int i;
  char marshalled[BIG_DUMB_NUMBER];
  printf("mmmmmmmmm before marshal_status\n");
  char status_line[BIG_DUMB_NUMBER];
  marshal_status(header, status_line);
  strcpy(buf, status_line);

  printf("mmmmmmmmm marshal header fields\n");
  for (i = 0; i < header->field_count; i++) {
    marshal_response_header_field(&(header->fields[i]), marshalled);
    strcat(buf, marshalled);
  }
}

void marshal_status(Response_header *header, char *buf) {
  printf("mmmmmmmmm marshal_status\n");
  char reason[BIG_DUMB_NUMBER];
  get_reason(header->status_code, reason);
  char status[3];
  sprintf(status, "%d", header->status_code);

  strcpy(buf, header->http_version);
  strcat(buf, " ");
  strcat(buf, status);
  strcat(buf, " ");
  strcat(buf, reason);
  strcat(buf, "\n");
  printf("mmmmmmmmm return marshalled status\n");
}

void marshal_response_header_field(Response_header_field *field, char* buf) {
  printf("mmmmmmmmm marshal_response_header_field\n");
  strcpy(buf, field->name);
  strcat(buf, ": ");
  strcat(buf, field->value);
  strcat(buf, "\n");
}

void get_reason(int status_code, char *reason) {
  printf("mmmmmmmmm get_reason\n");
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
}
