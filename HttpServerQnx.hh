/*
 * HttpServerQnx.hh
 *
 *  Created on: 9 déc. 2012
 *      Author: nuts
 */

#ifndef HTTPSERVERQNX_HH_
#define HTTPSERVERQNX_HH_

#include "tools.hh"

# define ISspace(x) isspace((int)(x))

#define ROOT_DIR "/root/_SiteQNX%s"
#define MAX_THREAD 5
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define PORT 8080
#define GMT 1

volatile int nb_sleeping;
volatile int client;

pthread_mutex_t mutex;
pthread_cond_t  nb_changed;
pthread_cond_t  client_changed;

void *accept_request(void*);
void serve_file(int, const char *);
int startup();
void loop(int);
void* thread_manager(void*);

#endif /* HTTPSERVERQNX_HH_ */
