#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>
#include <pthread.h>

#define PORT    5656
#define MAXMSG  512
/* stage 0: local listen
 * stage 1: check local listen
 * stage 2: create ssh connection
 * stage 3: check ssh connection established
 * stage 4: check other clients connecting to this port 
 */
static int stage = 0;

static int read_from_client(int filedes)
{
	char buffer[MAXMSG];
	int nbytes;

	nbytes = read(filedes, buffer, MAXMSG);
	if (nbytes < 0) {
		/* Read error. */
		perror("read");
		exit(EXIT_FAILURE);
	} else if (nbytes == 0)
		/* End-of-file. */
		return -1;
	else {
		/* Data read. */
		fprintf(stderr, "Server: got message: `%s'\n", buffer);
		return 0;
	}
}

static int make_socket(uint16_t port)
{
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sock;
}

void *listen_input(void *data)
{
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	size_t size;

	/* Create the socket and set it up to accept connections. */
	sock = make_socket(PORT);
	if (listen(sock, 1) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Initialize the set of active sockets. */
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	while (1) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror ("select");
			exit (EXIT_FAILURE);
		}

		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					/* Connection request on original socket. */
					int new;
					size = sizeof(clientname);
					new = accept(sock, 
							(struct sockaddr *) &clientname,
							&size);
					if (new < 0) {
						perror("accept");
						exit(EXIT_FAILURE);
					}
					fprintf(stderr,
							"Server: connect from host %s, port %hd.\n",
							inet_ntoa(clientname.sin_addr),
							ntohs(clientname.sin_port));
					FD_SET(new, &active_fd_set);

					/* if a new connection, local listen should exit 
 					*  and create the secure connection.
 					*/
					close(i);
					stage = 2;
					pthread_exit(0);
				} else {
					/* Data arriving on an already-connected socket. */
					if (read_from_client(i) < 0) {
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				}
			}
		}
	}
}

static void begin_listen()
{
	pthread_t handle;
	pthread_create(&handle, NULL, listen_input, NULL);
	if (!pthread_detach(handle)) {
		fprintf(stderr, "Thread detached successfully !!!\n");
	}
}

/* 5 seconds */
#define CHECK_INTERVAL 5

/* ssh connection script path */
#define SSH_CONNECTION_PATH "/root/ssh_connection.sh"

int main()
{

	/*main thread*/
	while (1) {
		switch(stage) {
			case 0:
				/* create local listen */ 
				begin_listen();
				/* check local listen */
				stage = 1;
				break;
			case 1:
			{
				/* check local listen 
				 * netstat -napt | grep 5656 | grep -v grep
				 */
				char cmdline[256] = {0}, buf[BUFSIZ] = {0};
				FILE *pfp = NULL;

				sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
				pfp = popen(cmdline, "r");
				if (NULL == pfp) {
					fprintf(stderr, "popen error!\n");
					exit(0);
				}

				while (fgets(buf, BUFSIZ, pfp) != NULL) {
					fprintf(stderr, "buf:%s\n", buf);
					/* check LISTEN */
					if (!strstr(buf, "LISTEN")) {
						fprintf(stderr, "LISTEN!\n");
						stage = 0;
						break;
					}
					memset(buf, 0, BUFSIZ);
				}

				pclose(pfp);
				pfp = NULL;
				break;
			}
			case 2:
			{
				/* create ssh connection */
				FILE *pfp = NULL;
				char cmdline[256] = {0}, buf[BUFSIZ] = {0};
				sprintf(cmdline, "sh %s", SSH_CONNECTION_PATH);
				pfp = popen(cmdline, "r");
				if (pfp == NULL) {
					fprintf(stderr, "popen error!\n");
					exit(0);
				}

				while (fgets(buf, BUFSIZ, pfp) != NULL) {
					fprintf(stderr, "buf:%s\n", buf);
					memset(buf, 0, BUFSIZ);
				}
				pclose(pfp);
				pfp = NULL;
				stage = 3;
				break;
			}
			case 3:
				/* check ssh connection */
				break;
			case 4:
				/* check other client connecting to this port */
			default:
				fprintf(stderr, "stage is set wrong value!\n");
		}
		sleep(CHECK_INTERVAL);
	}
}
