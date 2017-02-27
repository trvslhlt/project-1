#include <time.h>
#include <unistd.h>

// imports for parse
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"
#include "http.h"

typedef enum { false, true } bool;

// helper functions for assembling strings
char* get_reason(int);
char* assemble_status_line(int);
char* get_response(Request *, char *, bool);
char* make_header(char *, char *);
char* get_header_value(Request *, char *);
char* get_mime_type(char *);

// this function parses the request in request_buf and creates a response, which it writes to response
int http_handle_data(char *request_buf, int size, int socket_fd, char *response_buf) {
  
  // TODO: Check to see if the data in the request_buf completes a request before
  // attempting to create a Request struct from the data
  
  Request *request = parse(request_buf, size, socket_fd);
  if (!request) {
    return -1;
  }
  char *method = request->http_method;
  char *header = malloc(4096); // TODO: **** ADDRESS THIS
  char *entity_buffer = malloc(10000); //TODO: AND THIS

  if (strcmp(method,"GET") == 0) {
    printf("GET request made\n");
    header = get_response(request, entity_buffer, true);
    strcpy(response_buf, header); // start with the headers
    strcat(response_buf, entity_buffer); // add the actual content of the response
    strcat(response_buf, "\r\n"); // to end response
  }
  
  if (strcmp(method,"POST") == 0) {
    printf("POST request made\n");
  }
  
  if (strcmp(method,"HEAD") == 0) {
    printf("HEAD request made\n");
    header = get_response(request_buf, entity_buffer, false);
    strcpy(response_buf, header);
  }
  //TODO: handle other methods (by generating 501)
  return 0;
}

// helper function to get reason strings
char* get_reason(int status_code) {
  char *reason;
  switch (status_code) {
    case 200:
      reason = "OK";
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

// this function returns the headers, and if the request is a GET,
// it will also write the file content to entity_buffer (for head, will skip this step)
char* get_response(Request *request, char *entity_buffer, bool head_only) {
  char *header = malloc(4096);
  char *statusline = malloc(4096);
  FILE *file_requested;

  // time formatting code, from http://stackoverflow.com/questions/7548759/generate-a-date-string-in-http-response-date-format-in-c
  char timebuf[1000];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

  // status line creation
  if(strcmp(request->http_version, "HTTP/1.1") != 0) { // throw 505 if client is using diff HTTP version
    statusline = assemble_status_line(505);
    strcpy(header, statusline);
  } else if (access(request->http_uri + 1, F_OK) == -1) { // throw 404 if file not accessible
    statusline = assemble_status_line(404);
    strcpy(header, statusline);
  } else {
    file_requested = fopen(request->http_uri + 1, "r"); // open the file requested

    // get file size
    fseek(file_requested, 0L, SEEK_END);
    int file_size = ftell(file_requested);
    fseek(file_requested, 0L, SEEK_SET);

    char size_str[200];
    sprintf(size_str, "%d", file_size); // convert file size to a string

    char *mime = malloc(30);

    // get the file extension
    char *ext = strrchr(request->http_uri + 1, '.');

    // determine MIME type
    if (!ext) { // null extension
        strcpy(mime, "unknown");
    } else { // get mime type from helper function
        strcpy(mime, get_mime_type(ext+1));
    }

    // if we're getting more than head (haha), copy the file into our entity buffer
    if(head_only == true) {
      fread(entity_buffer, file_size, 1, file_requested); // WARNING: entity_buffer is allocated 10000 bytes before it is passed in,
                                                          // so this copy is unsafe if the file is larger than this
    }

    fclose(file_requested);

    // get modified time
    struct stat attr;
    stat(request->http_uri + 1, &attr);

    // save last modified time to string
    char modtimebuf[200];
    time_t modified_time = attr.st_mtime;
    struct tm tmx = *gmtime(&modified_time);
    strftime(modtimebuf, sizeof(modtimebuf), "%a, %d %b %Y %H:%M:%S %Z", &tmx);

    statusline = assemble_status_line(200); // generate status line OK

    // assemble the headers
    strcpy(header, statusline);
    strcat(header, make_header("Content-Length", size_str));
    strcat(header, make_header("Content-Type", mime));
    strcat(header, make_header("Modified-Time", modtimebuf));
  }

  strcat(header, make_header("Connection", "keep-alive"));
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

char* get_header_value(Request * request, char *name) {
    int index = 0;

    for(index = 0; index < request->header_count; index++) {
      if(strcmp(request->headers[index].header_name, name) == 0) {
        return request->headers[index].header_value;
      }
    }
    
    char *nullstr = "NULL";
    return nullstr;
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
