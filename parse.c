#include "parse.h"


#define BUF_SIZE 8192

int parse(char *buffer, int size, Request *request) {
  printf("pppppppp: parse: %s\n", buffer);
  enum { //Differant states in the state machine
    STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
  };

  int i = 0, state;
  size_t offset = 0;
  char ch;
  char buf[BUF_SIZE];
  memset(buf, 0, BUF_SIZE);

  printf("pppppppp: state machine\n");
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
  printf("pppppppp: valid end state\n");
  if (state == STATE_CRLFCRLF) {
    request->header.field_count = 0;
    //TODO You will need to handle resizing this in parser.y

    printf("pppppppp: before assigning header fields memory\n");
    request->header.fields = (Request_header_field *) malloc(sizeof(Request_header_field) * 1);
    printf("pppppppp: after assigning\n");
    set_parsing_options(buf, i, request);
    printf("pppppppp: before yyparse\n");
    if (yyparse() == SUCCESS) {
      printf("pppppppp: after yyparse succeeded\n");
      return 0;
    } else { // **********
      printf("pppppppp: after yyparse failed\n");
      return 0; // just trying something
    }
  }
  //TODO Handle Malformed Requests
  printf("pppppppp: not a valid end_state\n");
  return -1;
}



int invalid_request_data(char *request_data) {
  //TODO: implement
  return 0;
}

int complete_request(char *request_data) {
  // TODO: implement
  return 1;
}
