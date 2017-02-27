/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// imports for parse
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

typedef enum { false, true } bool;

int close_socket(int);
void cleanup_socks(int, int);
void interrupt_handler(int);

void parse_request(char *, int, int, char *);

// helper functions for assembling strings
char * get_reason(int);
char * assemble_status_line(int);
char * get_response(Request *, char *, bool);
char * make_header(char *, char *);
char * get_header_value(Request *, char *);
char * get_mime_type(char *);

FILE *log_file;

int main(int argc, char* argv[]) {
  int server_sock, min_sock, max_sock, new_sock;
  fd_set master_set, read_set;
  socklen_t cli_size;
  struct sockaddr_in addr, cli_addr;
  char buf[BUF_SIZE];
  int i;
  ssize_t nbytes;

  signal(SIGINT, interrupt_handler);
  log_file = fopen( "log.txt", "w" ); // Open file for writing
  if (log_file == NULL) {
    fprintf(stdout, "log file not available");
    return EXIT_FAILURE;
  }

  fprintf(log_file, "----- Echo Server -----\n");
  if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Failed creating socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(log_file, "server socket created: %d.\n", server_sock);
  FD_ZERO(&master_set);
  FD_ZERO(&read_set);
  FD_SET(server_sock, &master_set);
  min_sock = server_sock;
  max_sock = server_sock;

  // configure socket for reuse before bind
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    cleanup_socks(min_sock, max_sock);
    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed.\n");
    return EXIT_FAILURE;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ECHO_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_sock, (struct sockaddr *) &addr, sizeof(addr))) {
    cleanup_socks(min_sock, max_sock);
    fprintf(stderr, "Failed binding socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(log_file, "server socket bound.\n");

  if (listen(server_sock, 5)) {
    cleanup_socks(min_sock, max_sock);
    fprintf(stderr, "Error listening on socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(log_file, "server socket listening.\n");

  for(;;) {
    read_set = master_set;
    fprintf(log_file, "waiting for event on select.\n");
    if (select(max_sock + 1, &read_set, NULL, NULL, NULL) == -1) { //block until we have an event
      fprintf(stderr, "Error exiting select.\n");
      return EXIT_FAILURE;
    }

    for (i = min_sock; i <= max_sock; i++) {
      fprintf(log_file, "select event. checking socket: %d.\n", i);

      if (FD_ISSET(i, &read_set)) { // check for events on the socket fd
        if (i == server_sock) { // we have a connection request
          fprintf(log_file, "connection request.\n");
          cli_size = sizeof(cli_addr);
          if ((new_sock = accept(server_sock, (struct sockaddr *) &cli_addr, &cli_size)) == -1) {
            cleanup_socks(min_sock, max_sock);
            fprintf(stderr, "Error accepting connection.\n");
            return EXIT_FAILURE;
          } else {
            fprintf(log_file, "connection accepted on socket: %d", new_sock);
            FD_SET(new_sock, &master_set);
            if(new_sock > max_sock) {
              max_sock = new_sock;
            }
          }

        } else { // we have an event on an existing connection
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            if (nbytes == 0) { // connection closed
              fprintf(log_file, "connection closed on socket: %d\n", i);
            } else {
              fprintf(stderr, "Error reading data from socket: %d, error: %zd.\n", i, nbytes);
              return EXIT_FAILURE;
            }
            close_socket(i);
            FD_CLR(i, &master_set);
          } else { // we have data to read
            printf("%s\n", buf);
            // send to parsing function
            // TODO: ******* ADDRESS THIS
            char * response = malloc(10000);
            parse_request(buf, nbytes, i, response);
            memset(buf, 0, BUF_SIZE);
            
            if (send(i, response, strlen(response), 0) != strlen(response)) {
              cleanup_socks(min_sock, max_sock);
              fprintf(stderr, "Error sending to client.\n");
              return EXIT_FAILURE;
            }

            fprintf(log_file, "sent data to client: %d\n", i);
            

            free(response);
          }
        }
      } // check if socket file descriptor is in the set from select (has an event)
    } // iterating over socket file descriptors
  } // infinite for loop

  cleanup_socks(min_sock, max_sock);
  fclose(log_file);
  return EXIT_SUCCESS;
}

int close_socket(int sock) {
  if (close(sock)) {
    fprintf(stderr, "Failed closing socket.\n");
    return 1;
  }
  return 0;
}

void cleanup_socks(int min_sock, int max_sock) {
  int i;
  for (i = min_sock; i <= max_sock; i++) {
    close_socket(i);
  }
}

void interrupt_handler(int signal) {
  fclose(log_file);
}

// this function parses the request in request_buf and creates a response, which it writes to response
void parse_request(char * request_buf, int size, int socketFd, char * response) {
  Request * request = parse(request_buf, size, socketFd);

  char * method = request->http_method;

  char * header = malloc(4096); // **** ADDRESS THIS
  char * entity_buffer = malloc(10000); // AND THIS

  if (strcmp(method,"GET") == 0) {
    printf("GET request made\n");
    header = get_response(request, entity_buffer, true);

    strcpy(response, header); // start with the headers
    strcat(response, entity_buffer); // add the actual content of the response
    strcat(response, "\r\n"); // to end response
  }
  if (strcmp(method,"POST") == 0) {
    printf("POST request made\n");
  }
  if (strcmp(method,"HEAD") == 0) {
    printf("HEAD request made\n");
    header = get_response(request, entity_buffer, false);
    strcpy(response, header);
  }
  // **** need to handle other methods (by generating 501)
}


// helper function to get reason strings
char * get_reason(int status_code) {
  char * reason;
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
  }

  return reason;
}

// this function returns the headers, and if the request is a GET,
// it will also write the file content to entity_buffer (for head, will skip this step)
char * get_response(Request * request, char * entity_buffer, bool head_only) {
  char * header = malloc(4096);
  char * statusline = malloc(4096);
  FILE * file_requested;

  // time formatting code, from http://stackoverflow.com/questions/7548759/generate-a-date-string-in-http-response-date-format-in-c
  char timebuf[1000];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

  // status line creation
  if(strcmp(request->http_version, "HTTP/1.1") != 0) { // throw 505 if client is using diff HTTP version
    statusline = assemble_status_line(505);
    strcpy(header, statusline);
  }
  else if (access(request->http_uri + 1, F_OK) == -1) { // throw 404 if file not accessible
    statusline = assemble_status_line(404);
    strcpy(header, statusline);
  }
  else {
    file_requested = fopen(request->http_uri + 1, "r"); // open the file requested

    // get file size
    fseek(file_requested, 0L, SEEK_END);
    int file_size = ftell(file_requested);
    fseek(file_requested, 0L, SEEK_SET);

    char size_str[200];
    sprintf(size_str, "%d", file_size); // convert file size to a string

    char * mime = malloc(30);

    // get the file extension
    char * ext = strrchr(request->http_uri + 1, '.');

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

char * assemble_status_line(int status_code) {
  char * status_line = malloc(500); // ********* may be overkill
  char * http_version = "HTTP/1.1";

  char * reason = get_reason(status_code);

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
char * get_mime_type(char * extension) {
  char * mime_type = malloc(50);

  if(strcmp(extension, "html") == 0) {
    strcpy(mime_type, "text/html");
  }
  else if(strcmp(extension, "css") == 0) {
    strcpy(mime_type, "text/css");
  }
  else if(strcmp(extension, "png") == 0) {
    strcpy(mime_type, "image/png");
  }
  else if(strcmp(extension, "jpeg") == 0) {
    strcpy(mime_type, "image/jpeg");
  }
  else if(strcmp(extension, "gif") == 0) {
    strcpy(mime_type, "image/gif");
  }
  else {
    strcpy(mime_type, "unknown");
  }

  return mime_type;
}

char * get_header_value(Request * request, char * name) {
    int index = 0;

    for(index = 0; index < request->header_count; index++) {
      if(strcmp(request->headers[index].header_name, name) == 0) {
        return request->headers[index].header_value;
      }
    }

    // if header not set, return special string
    char * nullstr = "NULL";
    return nullstr;
}

// helper to create a new header field
char * make_header(char * name, char * value) {
  char * header = malloc(2096);

  strcpy(header, name);
  strcat(header, ": ");
  strcat(header, value);
  strcat(header, "\r\n");

  return header;
}
