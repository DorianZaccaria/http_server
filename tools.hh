/*
 * tools.hh
 *
 *  Created on: 9 déc. 2012
 *      Author: nuts
 */

#ifndef TOOLS_HH_
#define TOOLS_HH_

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define SERVER_STRING "Server: papoose/0.1.0\r\n"

void cat(int, FILE *);
void error_die(const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void unimplemented(int);

#endif /* TOOLS_HH_ */
