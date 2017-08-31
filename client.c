#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "common.h"
#include "client.h"

int len;
int sock_fd;
struct sockaddr_in address;
static char *buf;
static int cli_flag;

int main(int argc, char **argv)
{
	char cmd[MAX_LINE], arg[MAX_LINE];

	/* Command line parameters */
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
	address.sin_addr.s_addr = inet_addr("192.168.1.6");
	address.sin_port = htons(PORT);
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

		switch (opt) {
		case 's':
			stats_cl(&data, NULL);
			break;
		case 'a':
			add_cl(&data, optarg);
			break;
		case 'd':
			del_cl(&data, optarg);
			break;
		case 'p':
			padd_cl(&data, optarg);
			break;
		case 'r':
			pread_cl(&data, NULL);
			break;
		case 'c':
			cli_flag = 1;
			continue;
		case 't':
			ping_cl(&data, NULL);
			break;
		}

		/* The server must send something back, until a timeout
		 * occurs.
		 */
		get_ans_sync(&data);
	}

	/* Start command line mode, if the case */

	while (cli_flag) {
		struct net_data data;
		struct command *cmdp;

		printf(">");
		
		/* Parse input, retrieving command and argument */
		switch(parse_line(cmd, arg)) {
		case -EINVAL:
			/* Unbalanced double quotes or input too long */
			printf("Invalid input format\n");
			continue;
		case -EAGAIN:
			/* No input */
			continue;
		}

		/* Check whether cmd is a valid command */
		cmdp = get_cmdp(cmd, arg);
		if (cmdp == NULL) {
			printf("Invalid command\n");
			continue;
		}

		/* Send request and get answer */
		cmdp->fc(&data, arg);
		get_ans_sync(&data);
	}

	return 0;
}
