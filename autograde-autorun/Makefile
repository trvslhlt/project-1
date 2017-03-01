################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Project 1, for CS 5450    #
#                                                                              #
# Authors: Yuchen Wu <yuchenw@cs.cmu.edu>                                      #
# Authors: Eugene Bagdasaryan <ebagdasa@cs.cornell.edu>                        #
#                                                                              #
################################################################################

all:
	@rm -rf project-1
	@tar -mxf autograde.tar
	@tar -mxf handin.tar
	@mkdir -p apachebench
	@cp ./ab ./apachebench
	@perl -pi -e 's/HTTP\/1.0/HTTP\/1.1/g' ./apachebench/ab
	mkdir -p tmp
	cp -r www tmp
	@cp *.py project-1
	@cp -r unittest2 project-1
	@cp -r http_parser project-1
	@cp -r requests project-1
	@cp ./yacc project-1
	sed -i 's/yacc/.\/yacc/g' project-1/*akefile	
	cd project-1; python ./grader1cp2.py
