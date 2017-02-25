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

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int close_socket(int);

int main(int argc, char* argv[]) {
  int server_sock, min_sock, max_sock, new_sock;
  fd_set master_set, read_set;
  socklen_t cli_size;
  struct sockaddr_in addr, cli_addr;
  char buf[BUF_SIZE];
  int i;
  ssize_t nbytes;

  fprintf(stdout, "----- Echo Server -----\n");
  if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Failed creating socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "server socket created: %d.\n", server_sock);
  FD_ZERO(&master_set);
  FD_ZERO(&read_set);
  FD_SET(server_sock, &master_set);
  min_sock = server_sock;
  max_sock = server_sock;

  // configure socket for reuse before bind
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    close_socket(server_sock);
    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed.\n");
    return EXIT_FAILURE;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ECHO_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_sock, (struct sockaddr *) &addr, sizeof(addr))) {
    close_socket(server_sock);
    fprintf(stderr, "Failed binding socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "server socket bound.\n");

  if (listen(server_sock, 5)) {
    close_socket(server_sock);
    fprintf(stderr, "Error listening on socket.\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "server socket listening.\n");

  for(;;) {
    read_set = master_set;
    fprintf(stdout, "waiting for event on select.\n");
    if (select(max_sock + 1, &read_set, NULL, NULL, NULL) == -1) { //block until we have an event
      fprintf(stderr, "Error exiting select.\n");
      return EXIT_FAILURE;
    }

    for (i = min_sock; i <= max_sock; i++) {
      fprintf(stdout, "select event. checking socket: %d.\n", i);

      if (FD_ISSET(i, &read_set)) { // check for events on the socket fd
        if (i == server_sock) { // we have a connection request
          fprintf(stdout, "connection request.\n");
          cli_size = sizeof(cli_addr);
          if ((new_sock = accept(server_sock, (struct sockaddr *) &cli_addr, &cli_size)) == -1) {
            close(server_sock);
            fprintf(stderr, "Error accepting connection.\n");
            return EXIT_FAILURE;
          } else {
            fprintf(stdout, "connection accepted on socket: %d", new_sock);
            FD_SET(new_sock, &master_set);
            if(new_sock > max_sock) {
              max_sock = new_sock;
            }
          }

        } else { // we have an event on an existing connection
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            if (nbytes == 0) { // connection closed
              fprintf(stdout, "connection closed on socket: %d\n", i);
            } else {
              fprintf(stderr, "Error reading data from socket: %d, error: %zd.\n", i, nbytes);
              return EXIT_FAILURE;
            }
            close_socket(i);
            FD_CLR(i, &master_set);
          } else { // we have data to read
            if (send(i, buf, nbytes, 0) != nbytes) {
              //TODO: close all sockets
              close_socket(i);
              close_socket(server_sock);
              fprintf(stderr, "Error sending to client.\n");
              return EXIT_FAILURE;
            }
            fprintf(stdout, "sent data to client: %d\n", i);
            memset(buf, 0, BUF_SIZE);
          }
        }
      } // check if socket file descriptor is in the set from select (has an event)
    } // iterating over socket file descriptors
  } // infinite for loop


  close_socket(server_sock);
  return EXIT_SUCCESS;
}

int close_socket(int sock) {
  if (close(sock)) {
    fprintf(stderr, "Failed closing socket.\n");
    return 1;
  }
  return 0;
}
