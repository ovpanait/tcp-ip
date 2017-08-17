#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "common.h"
#include "ovsv.h"

static char debug_mode = 0;

int main(int argc, char **argv)
{
	int ret;
	
	/* Server-Client */
	
	int server_fd, client_fd;
	int server_len, client_len;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	/* Kernel module files */

	int page_size;
	int fd_arr[FD_ARR_COUNT];
	char *buf;
	
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

	/* Init server */
	
	page_size = getpagesize();
	server_init(fd_arr);
	
	if (debug_mode)
		printf("Debug mode activated.\n");
	
	/* Server side */
		       
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(9734);
	server_len = sizeof(server_addr);

	if (bind(server_fd, (struct sockaddr *)&server_addr, server_len) != 0) {
		perror("Bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, QUEUE_SIZE) != 0) {
		perror("Listen");
		exit(EXIT_FAILURE);
	}

	while(1) {
		char ch;
		
		printf("Server waiting.\n");

		client_len = sizeof(client_addr);
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
				   &client_len);

		read(client_fd, &ch, 1);
		ch++;
		write(client_fd, &ch, 1);
		close(client_fd);
		
	}
	
	return 0;
}
