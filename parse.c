#include "parse.h"


#define BUF_SIZE 8192

int parse(char *buffer, int size, Request *request) {
  
  enum { //Differant states in the state machine
    STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
  };

  int i = 0, state;
  size_t offset = 0;
  char ch;
  char buf[BUF_SIZE];
  memset(buf, 0, BUF_SIZE);

  state = STATE_START;
  while (state != STATE_CRLFCRLF) {
    char expected = 0;

    if (i == size) {
      break;
    }


    ch = buffer[i++];
    buf[offset++] = ch;

    switch (state) {
    case STATE_START:
    case STATE_CRLF:
      expected = '\r';
      break;
    case STATE_CR:
    case STATE_CRLFCR:
      expected = '\n';
      break;
    default:
      state = STATE_START;
      continue;
    }

    if (ch == expected) {
      state++;
    } else {
      state = STATE_START;
    }
  }

  //Valid End State
  if (state == STATE_CRLFCRLF) {
    request->header_count = 0;
    //TODO You will need to handle resizing this in parser.y
    request->headers = (Request_header *) malloc(sizeof(Request_header) * 1);
    set_parsing_options(buf, i, request);

    if (yyparse() == SUCCESS) {
      return 0;
    } else { // **********
      return -1;
    }
  }
  //TODO Handle Malformed Requests
  printf("Parsing Failed\n");
  return -1;
}
