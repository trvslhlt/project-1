#!/usr/bin/env bash

mkdir project-1

cp -t project-1 .git http.c http.h lexer.l lisod.c log.txt Makefile marshal.c marshal.h parse.c parse.h parser.y plcommon.py
tar -cf handin.tar project-1 > /dev/null

rm -rf project-1

mv handin.tar autograde-autorun
cd autograde-autorun

make
