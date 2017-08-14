#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ovsrvcli.h"

int main(int arcg, char **argv)
{
	int ret;
	
	int server_fd, client_fd;
	int server_len, client_len;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

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
