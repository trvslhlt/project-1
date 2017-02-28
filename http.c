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
Response get_default_response(int status_code);
int handle_request(Request *request, Response *response);
char* get_reason(int);
char* assemble_status_line(int);
char* get_response(Request *, char *, int);
char* make_header(char *, char *);
char* get_mime_type(char *);
int set_serve_folder(char *);
int set_continue(bool);
int get_method(char *http_method_str);

char *permanent_serve_folder;
bool following_continue; // bool to keep track of POSTs (send continue then OK)
Request *last_post_req;


int http_handle_data(char *request_buf, int size, int socket_fd, char *response_buf) {
  Request request;
  Response response;
  char *marshalled_response;
  // TODO: get existing data for socket_fd
  char *existing_data = malloc(BIG_DUMB_NUMBER);
    
  // update existing data
  strcpy(existing_data, request_buf);
  
  // check if existing data is invalid
  if (invalid_request_data(existing_data)) {
    free(existing_data);
    response = get_default_response(400);
    marshalled_response = marshal_response(&response);
    strcpy(response_buf, marshalled_response);
    free(marshalled_response);
    return strlen(response_buf);
  }
  
  // check if existing data is a complete request
  if (!complete_request(existing_data)) {
    // TODO: don't throw away data, wait for next event on socket
    free(existing_data);
    return 0;
  }

  if (parse(existing_data, size, &request) != 0) {
    free(existing_data);
    response = get_default_response(400);
    marshalled_response = marshal_response(&response);
    strcpy(response_buf, marshalled_response);
    free(marshalled_response);
    return strlen(response_buf);
  }
  // TODO: don't throw away data, wait for next event on socket
  free(existing_data);
  
  if (handle_request(&request, &response) != 0) {
    get_default_response(500);
  }
  
  // TODO: delete persistent store of existing_data
  free(existing_data);
  marshalled_response = marshal_response(&response);
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
  
  switch (method) {
    case GET:
      *response = get_default_response(500);
      break;
    case HEAD:
      *response = get_default_response(500);
      break;
    case POST:
      *response = get_default_response(500);
      break;
    default:
      *response = get_default_response(501);
  }
  return 0;

  // char *method = request.http_method;
  // 
  // if (strcmp(method,"GET") == 0) {
  //   printf("GET request made\n");
  //   strcpy(header, get_response(request, entity_buffer, GET));
  //   strcpy(response_buf, header); // start with the headers
  //   strcat(response_buf, entity_buffer); // add the actual content of the response
  //   strcat(response_buf, "\r\n"); // to end response
  // }
  // else if (strcmp(method,"POST") == 0) {
  //   printf("POST request made\n");
  //   strcpy(header, get_response(request , entity_buffer, POST));
  //   strcpy(response_buf, header);
  //   following_continue = true;
  // }
  // else if (strcmp(method,"HEAD") == 0) {
  //   printf("HEAD request made\n");
  //   strcpy(header, get_response(request , entity_buffer, HEAD));
  //   strcpy(response_buf, header);
  // }
  // else {
  //   printf("Unimplemented method request made\n");
  // 
  //   strcat(header, assemble_status_line(501)); // return unimplemented line
  //   // standard headers
  //   strcat(header, make_header("Server", "Liso/1.0"));
  //   strcat(header, "\r\n"); // end of header line
  //   strcpy(response_buf, header);
  // }
  // 
  // free(header);
  // free(entity_buffer);
  // // free(request);
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

int set_serve_folder(char * serve_folder) {
  permanent_serve_folder = malloc(200);
  strcpy(permanent_serve_folder, serve_folder);
  return 0;
}

int set_continue(bool continue_status) {
  following_continue = continue_status;
  return 0;
}


// this function returns the headers, and if the request is a GET,
// it will also write the file content to entity_buffer (for head, will skip this step)
char* get_response(Request *request, char *entity_buffer, int method) {
  char *header = malloc(8192);
  char *statusline = malloc(500);
  FILE *file_requested;

  // time formatting code, from http://stackoverflow.com/questions/7548759/generate-a-date-string-in-http-response-date-format-in-c
  char timebuf[300];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

  char * filepath = malloc(300);
  strcpy(filepath, permanent_serve_folder);
  strcat(filepath, "/");
  strcat(filepath, request->header.uri + 1);

  // status line creation
  if(strcmp(request->header.http_version, "HTTP/1.1") != 0) { // throw 505 if client is using diff HTTP version
    strcpy(statusline, assemble_status_line(505));
    strcpy(header, statusline);
    strcat(header, make_header("Connection", "close"));
  } else if (access(filepath, F_OK) == -1) { // throw 404 if file not accessible
    strcpy(statusline, assemble_status_line(404));
    strcpy(header, statusline);
  }
 else {
    file_requested = fopen(filepath, "r"); // open the file requested

    // get file size
    fseek(file_requested, 0L, SEEK_END);
    int file_size = ftell(file_requested);
    fseek(file_requested, 0L, SEEK_SET);

    char * size_str = malloc(200);
    sprintf(size_str, "%d", file_size); // convert file size to a string

    char *mime = malloc(30);

    // get the file extension
    char *ext = strrchr(request->header.uri + 1, '.');

    // determine MIME type
    if (!ext) { // null extension
        strcpy(mime, "unknown");
    } else { // get mime type from helper function
        strcpy(mime, get_mime_type(ext+1));
    }

    // if we're getting more than head (haha), copy the file into our entity buffer
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

    printf("in herenow %d", following_continue);

    if((method == POST) && (following_continue == false)) { // send a continue
      printf("hello");
      strcpy(statusline, assemble_status_line(100));
      last_post_req = request;
    }
    else {
      strcpy(statusline, assemble_status_line(200)); // generate status line OK
    }

    // assemble the headers
    strcpy(header, statusline);
    if(method == GET) { // no need if the content is a post
      strcat(header, make_header("Content-Length", size_str));
      strcat(header, make_header("Content-Type", mime));
      strcat(header, make_header("Modified-Time", modtimebuf));
    }

    if(method == POST) {
      strcat(header, make_header("Content-Length", "0"));
    }

    strcat(header, make_header("Connection", "close"));

    // TODO: figure out how to free this
    // free(modtimebuf);
    // free(mime);
    // free(ext);
    // free(size_str);
  }

  strcat(header, make_header("Date", timebuf));

  // standard headers
  strcat(header, make_header("Server", "Liso/1.0"));
  strcat(header, "\r\n"); // end of header line

  free(statusline);

  return header;
}

char* assemble_status_line(int status_code) {
  char *status_line = malloc(500); // ********* may be overkill
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
  return (Response){header, "This is the test body of a bad request response.\n"};
}