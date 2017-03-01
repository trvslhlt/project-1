#include <time.h>
#include <unistd.h>

// imports for parse
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"
#include "http.h"
#include "marshal.h"

#define BIG_DUMB_NUMBER 10000

typedef enum { false, true } bool;
enum { GET = 0, HEAD, POST };

// helper functions for assembling strings
Response get_default_response(int);
Response get_custom_response(Request *, int);
int handle_request(Request *request, Response *response);
char* get_reason(int);
char* assemble_status_line(int);
char* make_header(char *, char *);
char* get_mime_type(char *);
int set_serve_folder(char *);
int set_continue(bool);
int get_method(char *http_method_str);

char *permanent_serve_folder;
bool following_continue; // bool to keep track of POSTs (send continue then OK)
Request *last_post_req;
char existing_data[BIG_DUMB_NUMBER];

int http_handle_data(char *request_buf, int size, int socket_fd, char *response_buf) {
  printf("----- http_handle_data\n");
  Request request;
  Response response;
  char *marshalled_response;

  // TODO: get existing data for socket_fd
  memset(existing_data, 0, sizeof(existing_data));
  printf("----- %zd\n", strlen(existing_data));
  // char *existing_data = malloc(BIG_DUMB_NUMBER);

  printf("----- update existing_data\n");
  // update existing data
  strcpy(existing_data, request_buf);

  printf("----- check if existing_data is valid\n");
  // check if existing data is invalid
  if (invalid_request_data(existing_data)) {
    // free(existing_data);
    response = get_default_response(400);
    marshalled_response = marshal_response(&response);
    strcpy(response_buf, marshalled_response);
    free(marshalled_response);
    return strlen(response_buf);
  }

  printf("----- check if complete request data\n");
  // check if existing data is a complete request
  if (!complete_request(existing_data)) {
    // TODO: don't throw away data, wait for next event on socket
    // free(existing_data);
    return 0;
  }

  printf("----- parse to request\n");
  if (parse(existing_data, size, &request) != 0) {
    printf("----- failed to parse: %s", existing_data);
    // free(existing_data);
    response = get_default_response(400);
    marshalled_response = marshal_response(&response);
    printf("%s\n", marshalled_response);
    strcpy(response_buf, marshalled_response);
    free(marshalled_response);
    return strlen(response_buf);
  }
  printf("----- finished parsing: %s", existing_data);

  printf("----- handle request\n");
  if (handle_request(&request, &response) != 0) {
    printf("----- failed to handle request\n");
    get_default_response(500);
  }

  printf("----- return the response\n");
  marshalled_response = marshal_response(&response);
  printf("----- response data: %s\n", marshalled_response);
  strcpy(response_buf, marshalled_response);
  free(marshalled_response);
  return strlen(response_buf);
}

int handle_request(Request *request, Response *response) {
  // TODO: handle request
  int method = get_method(request->header.method);
  if (method < 0) {
    *response = get_default_response(501);
    return 0;
  }

  *response = get_custom_response(request, method);

  // TODO: use below code for anything?
  // switch (method) {
  //   case GET:
  //     *response = get_default_response(500);
  //     break;
  //   case HEAD:
  //     *response = get_default_response(500);
  //     break;
  //   case POST:
  //     *response = get_default_response(500);
  //     break;
  //   default:
  //     *response = get_default_response(501);
  // }

  return 0;
}

int get_method(char *http_method_str) {
  if (strcmp(http_method_str, "GET") == 0) {
    return GET;
  } else if (strcmp(http_method_str, "HEAD") == 0) {
    return HEAD;
  } else if (strcmp(http_method_str, "POST") == 0) {
    return POST;
  } else {
    return -1;
  }
}

char* assemble_status_line(int status_code) {
  char *status_line = malloc(500);
  char *http_version = "HTTP/1.1";
  char *reason = get_reason(status_code);
  char status_str[3];
  sprintf(status_str, "%d", status_code);

  strcpy(status_line, http_version);
  strcat(status_line, " ");
  strcat(status_line, status_str);
  strcat(status_line, " ");
  strcat(status_line, reason);
  strcat(status_line, "\r\n");
  return status_line;
}

// helper function to get mime types based on extensions
char* get_mime_type(char *extension) {
  char *mime_type = malloc(50);

  if(strcmp(extension, "html") == 0) {
    strcpy(mime_type, "text/html");
  } else if(strcmp(extension, "css") == 0) {
    strcpy(mime_type, "text/css");
  } else if(strcmp(extension, "png") == 0) {
    strcpy(mime_type, "image/png");
  } else if(strcmp(extension, "jpeg") == 0) {
    strcpy(mime_type, "image/jpeg");
  } else if(strcmp(extension, "gif") == 0) {
    strcpy(mime_type, "image/gif");
  } else {
    strcpy(mime_type, "unknown");
  }
  return mime_type;
}

// helper to create a new header field
char* make_header(char *name, char *value) {
  char *header = malloc(2096);
  strcpy(header, name);
  strcat(header, ": ");
  strcat(header, value);
  strcat(header, "\r\n");
  return header;
}

Response get_default_response(int status_code) {
  Response_header header;
  strcpy(header.http_version, "HTTP/1.1");
  header.status_code = status_code;
  Response_header_field field;
  strcpy(field.name, "fieldName");
  strcpy(field.value, "fieldValue");
  Response_header_field fields[1] = {field};
  header.fields = fields;
  header.field_count = 1;
  return (Response){header, NULL};
}

Response get_custom_response(Request *request, int method) {
  char *entity_buffer = malloc(10000);
  Response_header header;
  int field_count = 0;
  FILE *file_requested;

  // time formatting code, from http://stackoverflow.com/questions/7548759/generate-a-date-string-in-http-response-date-format-in-c
  char timebuf[300];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

  char *filepath = malloc(300);
  strcpy(filepath, permanent_serve_folder);
  strcat(filepath, "/");
  strcat(filepath, request->header.uri + 1);

  strcpy(header.http_version, "HTTP/1.1");

  // default fields
  Response_header_field server_field;
  strcpy(server_field.name, "Server");
  strcpy(server_field.value, "Liso/1.0");
  field_count++;

  Response_header_field date_field;
  strcpy(date_field.name, "Date");
  strcpy(date_field.value, timebuf);
  field_count++;

  // status line creation
  if(strcmp(request->header.http_version, "HTTP/1.1") != 0) { // throw 505 if client is using diff HTTP version
    header.status_code = 505;
    Response_header_field connection_field;
    strcpy(connection_field.name, "Connection");
    strcpy(connection_field.value, "close");
    field_count++;
    Response_header_field fields[3] = {server_field, date_field, connection_field};
    header.fields = fields;
    header.field_count = field_count;
    return (Response){header, NULL};
  } else if (access(filepath, F_OK) == -1) { // throw 404 if file not accessible
    header.status_code = 505;
    Response_header_field fields[2] = {server_field, date_field};
    header.fields = fields;
    header.field_count = field_count;
    return (Response){header, NULL};
  } else {
    file_requested = fopen(filepath, "r"); // open the file requested

    // get file size
    fseek(file_requested, 0L, SEEK_END);
    int file_size = ftell(file_requested);
    fseek(file_requested, 0L, SEEK_SET);
    char *size_str = malloc(200);
    sprintf(size_str, "%d", file_size); // convert file size to a string
    char *mime = malloc(30);
    char *ext = strrchr(request->header.uri + 1, '.'); // get the file extension

    if (!ext) { // null extension
        strcpy(mime, "unknown");
    } else { // get mime type from helper function
        strcpy(mime, get_mime_type(ext+1));
    }

    // if we're getting more than head, copy the file into our entity buffer
    if(method == GET) {
      fread(entity_buffer, file_size, 1, file_requested); // WARNING: entity_buffer is allocated 10000 bytes before it is passed in,
                                                          // so this copy is unsafe if the file is larger than this
    }

    fclose(file_requested);

    // get modified time
    struct stat attr;
    stat(filepath, &attr);

    // save last modified time to string
    char modtimebuf[200];
    time_t modified_time = attr.st_mtime;
    struct tm tmx = *gmtime(&modified_time);
    strftime(modtimebuf, sizeof(modtimebuf), "%a, %d %b %Y %H:%M:%S %Z", &tmx);

    if((method == POST) && (following_continue == false)) { // send a continue
      header.status_code = 100;
      last_post_req = request;
    } else {
      header.status_code = 200;
    }

    // assemble the headers
    if((method == GET) || (method == HEAD)) { // no need if the content is a post
      Response_header_field content_length_field;
      strcpy(content_length_field.name, "Content-Length");
      strcpy(content_length_field.value, size_str);
      field_count++;

      Response_header_field content_type_field;
      strcpy(content_type_field.name, "Content-Type");
      strcpy(content_type_field.value, mime);
      field_count++;

      Response_header_field mod_time_field;
      strcpy(mod_time_field.name, "Modified-Time");
      strcpy(mod_time_field.value, modtimebuf);
      field_count++;

      Response_header_field fields[5] = {content_length_field, content_type_field, mod_time_field, server_field, date_field};
      header.fields = fields;
      header.field_count = field_count;

      if(method == GET) {
        return (Response){header, entity_buffer};
      } else {
        return (Response){header, NULL};
      }
    } else {
      Response_header_field content_length_field;
      strcpy(content_length_field.name, "Content-Length");
      strcpy(content_length_field.value, "0");
      field_count++;

      Response_header_field fields[3] = {content_length_field, server_field, date_field};
      header.fields = fields;
      header.field_count = field_count;

      return (Response){header, NULL};
    }
  }
}

int set_serve_folder(char *serve_folder) {
  permanent_serve_folder = malloc(200);
  strcpy(permanent_serve_folder, serve_folder);
  return 0;
}

int set_continue(bool continue_status) {
  following_continue = continue_status;
  return 0;
}