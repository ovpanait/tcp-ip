#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "helpers.h"

static char debug_mode;
static char *buf;

void termination_handler(int signum)
{
	server_clean(buf);
}

int main(int argc, char **argv)
{
	/* Auxiliary */
	pid_t pid;

	struct sigaction sig;
	sigset_t mask;

	/* Server-Client */
	int server_fd;
	int client_fd;
	socklen_t client_len;
	struct sockaddr_in client_addr;

	/* Command line */

	int opt;
	struct option longopts[] = {
		{"debug", 0, NULL, 'd'},
		{0, 0, 0, 0} };

	while ((opt = getopt_long_only(argc, argv, ":d", longopts, NULL)) != -1)
		switch (opt) {
		case 'd':
			debug_mode = 1;
			break;
		}

	/* Activate debug mode, if necessary */
	if (debug_mode)
		printf("Debug mode activated.\n");

	/* Allocate buffer of size PAGE_SIZE */
	page_size = getpagesize();
	buf = malloc(page_size);
	if (buf == NULL) {
		fprintf(stderr, "Error allocating %d bytes.\n", page_size);
		exit(EXIT_FAILURE);
	}
	buf[page_size - 1] = '\0';

	/* Shut down the server when a major error occurs.
	 * Child processes which encounter errors such as "missing kernel file"
	 * or i/o errors will send SIGTERM to the server.
	 */
	signal(SIGCHLD, SIG_IGN); /* Ignore child exit details */
	sig.sa_handler = &termination_handler; /* Cleanup handler */
	sigfillset(&mask); /* Block any other signals from occuring during cleanup.
			    * The signal currently being handled is also blocked.
			    */
	sig.sa_mask = mask;
	sigaction(SIGTERM, &sig, NULL);

	/* Server init */
	server_fd = server_init();

	while(1) {
		struct net_data data;

		printf("Server waiting.\n");

		client_len = sizeof(client_addr);
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
				   &client_len);

		pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		if (pid == 0) {
			/* Child will handle client request */
			if (get_net_data(&data, client_fd) != 0) {
				fprintf(stderr, "Error occurred on get_net_data.\n");
				/* TODO */
			}
			else
				switch (GET_CMD_ID(data.header)) {
				case LIST_ADD_ID:
					ladd_sv(&data, buf);
					break;
				case LIST_STATS_ID:
					stats_sv(&data, buf);
					break;
				case LIST_DEL_ID:
					ldel_sv(&data, buf);
					break;
				case PADD_ID:
					padd_sv(&data, buf);
					break;
				case PREAD_ID:
					pread_sv(&data, buf);
					break;
				case PING_ID:
					ping_sv(&data, buf);
					break;
				}

			close(client_fd);
			exit(EXIT_SUCCESS);
		}

		close(client_fd);
	}

	return 0;
}
