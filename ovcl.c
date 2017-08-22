#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "helpers.h"

#define MAX_FD 1024

static char *buf;
static int seq;
static int cli_flag;

int main(int argc, char **argv)
{
	/* Auxiliary */
	long int l;
	char *endptr;

	/* File descriptors */
	int len;
	int sock_fd;
	struct sockaddr_in address;
	int result;
	fd_set readfds;
	
	/* Command line parameter parsing */
	int opt;
	struct option longopts[] = {
		{"stats", 0, NULL, 's'},
		{"add", 1, NULL, 'a'},
		{"del", 1, NULL, 'd'},
		{"padd", 1, NULL, 'p'},
		{"pread", 0, NULL, 'r'},
		{"cli", 0, NULL, 'c'},
		{"ping", 0, NULL, 't'},
		{0, 0, 0, 0} };

	/*  Name the socket, as agreed with the server.  */

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(9734);
	len = sizeof(address);

	page_size = getpagesize();
	buf = malloc(page_size);
	if (buf == NULL) {
		fprintf(stderr, "Error allocating %d bytes.\n", page_size);
		exit(EXIT_FAILURE);
	}
	buf[page_size - 1] = '\0';
	
	/* Perform operations based on command line parameters */
	
	while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1 &&
	       (opt != '?')) {
		struct net_data data;
		
		sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		result = connect(sock_fd, (struct sockaddr *)&address, len);
		if(result == -1) {
			perror("connect");
			exit(EXIT_FAILURE);
		}

		switch (opt) {
		case 's':
			printf("Calling list_get_stats.\n");
			stats_rq(&data, sock_fd);
			break;
		case 'a':
			printf("Calling list_add.\n");
			add_rq(&data, optarg, sock_fd);
			break;
		case 'd':
			printf("Calling list_del.\n");
			/* Use strtol instead of atoi in order to avoid 
			 * undefined behavior;
			 */
			l = strtol(optarg, &endptr, 10);
			if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX) ||
			    l < INT_MAX || (errno == ERANGE && l == LONG_MIN) ||
			    *endptr != '\0') {
				fprintf(stderr, "Failed to convert del argument.\n");
				exit(EXIT_FAILURE);
			}
			seq = l;
			del_rq(&data, optarg, sock_fd);
			break;	
		case 'p':
			printf("Calling padd.\n");
			padd_rq(&data, optarg, sock_fd);
			break;
		case 'r':
			printf("Calling read.\n");
			pread_rq(&data, sock_fd);
			break;
		case 'c':
			printf("Calling cli.\n");
			cli_flag = 1;
			break;
		case 't':
			printf("Calling ping.\n");
			ping_rq(&data, sock_fd);
			break;
		}

		 get_ans_sync(&data);
	}
	
	if (cli_flag) {
		/* Start command line */
		/*  We can now read/write via sock_fd.  */
	}
	
	return 0;
}
