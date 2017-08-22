#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "helpers.h"

static char debug_mode = 0;

int main(int argc, char **argv)
{
	int ret;
	pid_t pid;
	char *buf;
		
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

	/* Activate debug mode, if necessary */
	if (debug_mode)
		printf("Debug mode activated.\n");

	/* Ignore child exit details */
	signal(SIGCHLD, SIG_IGN);
	
	while(1) {
		struct net_data data;
		int cmd_id;
		
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
			/* Child - handle client request */

			printf("Entered child process.\n");
			
			if (get_net_data(&data, client_fd) != 0) {
				fprintf(stderr, "Error occurred on get_net_data.\n");
			}
			else {
			       cmd_id =  GET_CMD_ID(data.header);
			       printf("Command id: %u\n", cmd_id);
			       switch (cmd_id) {
			       case LIST_ADD_ID:
				       printf("Starting ladd_send\n");
				       ladd_send(&data, buf);
				       break;
			       case LIST_STATS_ID:
				       stats_send(&data, buf);
				       break;
			       case LIST_DEL_ID:
				       ldel_send(&data, buf);
				       break;
			       case PADD_ID:
				       break;
			       case PREAD_ID:
				       break;
			       case PING_ID:
				       break;
			       }
			}
			
			close(client_fd);
			exit(EXIT_SUCCESS);
		}
		printf("Parent process: closing fd.\n");
		close(client_fd);
	}
	
	return 0;	
}
