
#include "HttpServerQnx.hh"

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void *accept_request(void* clt) {
	int client = *(int*) clt;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0; /* becomes true if server decides this is a CGI
	 * program */
	char *query_string = NULL;

	numchars = get_line(client, buf, sizeof(buf));
	printf("%s", buf);
	i = 0;
	j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET")) {
		unimplemented(client);
		return (void*) 0;
	}

	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0) {
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;
		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, ROOT_DIR, url);
	if (path[strlen(path) - 1] == '/')
		strcat(path, "index.html");
	if (stat(path, &st) == -1) {
		while ((numchars > 0) && strcmp("\n", buf))
			numchars = get_line(client, buf, sizeof(buf));
		not_found(client);
	} else {
		if ((st.st_mode & S_IFMT) == S_IFDIR)
			strcat(path, "/index.html");
		if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode
				& S_IXOTH))
			cgi = 1;
		if (!cgi)
			serve_file(client, path);
	}

	close(client);
	return (void*) 0;
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename) {
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	buf[0] = 'A';
	buf[1] = '\0';
	while ((numchars > 0) && strcmp("\n", buf))
		numchars = get_line(client, buf, sizeof(buf));

	resource = fopen(filename, "r");
	if (resource == NULL)
		not_found(client);
	else {
		headers(client, filename);
		cat(client, resource);
	}
	fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup() {
	struct sockaddr_in sin;
	int sock = 0;
	socklen_t recsize = sizeof(sin);
	int sock_err;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET)
		error_die("socket");

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sock_err = bind(sock, (struct sockaddr*) &sin, recsize);

	if (sock_err == SOCKET_ERROR)
		error_die("bind");

	sock_err = listen(sock, 5);
	printf("Listen on port %d...\n", PORT);

	if (sock_err == SOCKET_ERROR)
		error_die("listen");
	return (sock);
}

void *thread_manager(void *args)
{
	  errno = pthread_mutex_lock(&mutex);
	  if (errno)
		  error_die("pthread_mutex_lock failure");

	  while (1) {
	    while (client==-1) {
	      errno = pthread_cond_wait(&client_changed, &mutex);
	      if (errno)
	    	  error_die("pthread_cond_wait failure");
	    }
	    nb_sleeping--;

	    errno = pthread_mutex_unlock(&mutex);
	    if (errno)
	    	error_die("pthread_mutex_unlock failure");

	    accept_request((void*) &client);

	    client = -1;

	    errno = pthread_mutex_lock(&mutex);
	    if (errno)
	    	error_die("pthread_mutex_lock failure");
	    nb_sleeping++;
	    errno = pthread_cond_signal(&nb_changed);
	    if (errno)
	    	error_die("pthread_cond_signal failure");
	  }
	  return (void*) 0;
}

void loop(int sock)
{
	struct sockaddr_in csin;
	socklen_t crecsize = sizeof(csin);
	int csock = 0;

	printf("test debug\n");
	errno = pthread_mutex_lock(&mutex);
	if (errno)
		error_die("pthread_mutex_lock failure");

	while (true) {

		while (!nb_sleeping) {
		      errno = pthread_cond_wait(&nb_changed, &mutex);
		      if (errno)
		    	  error_die("pthread_cond_wait failure");
		    }

		printf(
				"Waiting for connection on port %d...\n",
				PORT);
		errno = pthread_mutex_unlock(&mutex);
		    if (errno)
		    	error_die("pthread_mutex_unlock failure");
		csock = accept(sock, (struct sockaddr*) &csin, &crecsize);

		errno = pthread_mutex_lock(&mutex);
		if (errno)
			error_die("pthread_mutex_unlock failure");
		client = csock;
		if (client == -1) {
			if (errno == EINTR || errno == ECONNABORTED)
				continue;
			error_die("accept failure");
			}
		errno = pthread_cond_signal(&client_changed);
		if (errno)
			error_die("pthread_cond_signal failure");
	}
	printf("Closing client socket\n");
	close(csock);
	printf("Closing server socket\n");
	close(sock);
}

int main(int argc, char *argv[]) {
	pthread_t thread;
	int i;

	client = -1;
	nb_sleeping = MAX_THREAD;
	errno = pthread_mutex_init(&mutex, NULL);
	if (errno)
		error_die("pthread_mutex_init failure");
	errno = pthread_cond_init(&nb_changed, NULL);
	if (errno)
		error_die("pthread_cond_init failure");
	errno = pthread_cond_init(&client_changed, NULL);
	if (errno)
		error_die("pthread_cond_init failure");
	for (i=0; i < MAX_THREAD; i++)  {
		errno = pthread_create(&thread, NULL, thread_manager, NULL);
		if (errno)
			error_die("pthread_create failure");
	  }
	printf("Init ... ok\n");
	loop(startup());
	return EXIT_SUCCESS;
}
