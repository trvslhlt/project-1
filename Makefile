CC=gcc
CFLAGS=-I.
DEPS = parse.h y.tab.h http.h
OBJ = y.tab.o lex.yy.o parse.o marshal.o http.o lisod.o
FLAGS = -g -Wall

default: lisod

lex.yy.c: lexer.l
	flex $^

y.tab.c: parser.y
	yacc -d $^

%.o: %.c $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

lisod: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	@rm -f lisod
	@rm -f *~ *.o example lex.yy.c y.tab.c y.tab.h
