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

int main(int argc, char **argv)
{
	/* Auxiliary */
	pid_t pid;
	char *buf;
	
	pid_t ret;
	int status;
	fd_set inputs, testfds;
	struct timeval timeout;
	
	/* Server-Client */

	int server_fd;
	int client_fd;
	int client_len;
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

	/* Allocate buffer of size PAGE_SIZE */
	page_size = getpagesize();
	buf = malloc(page_size);
	if (buf == NULL) {
		fprintf(stderr, "Error allocating %d bytes.\n", page_size);
		exit(EXIT_FAILURE);
	}
	buf[page_size - 1] = '\0';

	/* Server initialization */
	server_fd = server_init();
	FD_ZERO(&inputs);
	FD_SET(server_fd, &inputs);
	
	/* Activate debug mode, if necessary */
	if (debug_mode)
		printf("Debug mode activated.\n");
	
	while(1) {
		struct net_data data;
		int cmd_id;

		printf("Server waiting.\n");

		/* Check the exit status of the children, without blocking.
		 * Children will exiit with ENOENT when the kernel debugfs
		 * files don't exist, and with EIO when an i/o operation
		 * fails.
		 */
		errno = 0;
		ret = waitpid(-1, &status, WNOHANG);
		if (ret != 0 && errno != ECHILD) {
			if (ret == -1) {
				/* Error */
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
	
			if (WIFEXITED(status)) {
				/* A child finished normally */
				switch(WEXITSTATUS(status)){
				case ENOENT:
					/* Kernel module unloaded unexpectedly */
					fprintf(stderr, "Kernel files missing\n");
					exit(EXIT_FAILURE);
					break;
				case EIO:
					/* Read/Write error occurred */
					fprintf(stderr, "IO error\n");
					exit(EXIT_FAILURE);
					break;
				}
			}
			else {
				/* Child killed by signal. Shouldn't have happened */
				fprintf(stderr, "Child process killed by signal %d\n",
					WTERMSIG(status));
				exit(EXIT_FAILURE);
			}
		}

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		testfds = inputs;
		switch (select(server_fd + 1, &testfds, NULL, NULL, &timeout)) {
		case 0:
			/* No activity. Go back and check for errors */
			continue;
		case -1:
			perror("select");
			exit(EXIT_FAILURE);
			break;
		}

		/* No errors so far, we can accept the client and fork */
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
					ladd_send(&data, buf);
					break;
				case LIST_STATS_ID:
					stats_send(&data, buf);
					break;
				case LIST_DEL_ID:
					ldel_send(&data, buf);
					break;
				case PADD_ID:
					padd_send(&data, buf);
					break;
				case PREAD_ID:
					pread_send(&data, buf);
					break;
				case PING_ID:
					ping_send(&data, buf);
					break;
				}
			
			close(client_fd);
			exit(EXIT_SUCCESS);
		}

		close(client_fd);
	}
	
	return 0;	
}
