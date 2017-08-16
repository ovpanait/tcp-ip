/*  Make the necessary includes and set up the variables.  */

#include <sys/types.h>
#include <sys/socket.h>
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

#include "ovsrvcli.h"

static char *buf;
static int seq;
static int cli_flag;

int main(int argc, char **argv)
{
	int page_size;
	long int l;
	char *endptr;
	
	int sockfd;
	int len;
	struct sockaddr_in address;
	int result;
	char ch = 'A';

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

	/*  Create a socket for the client.  */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/*  Name the socket, as agreed with the server.  */

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(9734);
	len = sizeof(address);

	/*  Now connect our socket to the server's socket.  */

	result = connect(sockfd, (struct sockaddr *)&address, len);

	if(result == -1) {
		perror("oops: client3");
		exit(1);
	}

	page_size = getpagesize();
	buf = malloc(page_size);
	if (buf == NULL) {
		fprintf(stderr, "Error allocating %d bytes.\n", page_size);
		exit(EXIT_FAILURE);
	}
	buf[page_size - 1] = '\0';
	
	/* Perform operations based on command line parameters */
	
	while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1)
		switch (opt) {
		case 's':
			printf("Calling list_get_stats.\n");
			/*list_get_stats(); */
			break;
		case 'a':
			printf("Calling list_add.\n");
			/* list_add(buf); */
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
			/* list_del(seq); */
			break;	
		case 'p':
			printf("Calling padd.\n");
			/* page_add(buf); */
			break;
		case 'r':
			printf("Calling read.\n");
			/* page_read(); */
			break;
		case 'c':
			printf("Calling cli.\n");
			cli_flag = 1;
			break;
		case 't':
			printf("Calling ping.\n");
			/* ping(); */
			break;
		case ':':
			printf("Option needs value.\n");
			break;
		case '?':
			printf("unknown option %c\n", optopt);
			break;
		}

	if (cli_flag) {
		/* Start command line */
		/*  We can now read/write via sockfd.  */

		write(sockfd, &ch, 1);
		read(sockfd, &ch, 1);
		printf("char from server = %c\n", ch);
		close(sockfd);
	}
	
	return 0;
}
