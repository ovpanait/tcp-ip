#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>

#include "helpers.h"

int page_size;

/* Server side */

int server_init(void)
{
	int ret;
	int server_fd;
	int server_len;
	struct sockaddr_in server_addr;

	/* Change current directory */

	ret = chdir("/sys/kernel/debug/ovidiu");
	if (ret == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}
		
	/* Server info */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	server_len = sizeof(server_addr);

	if (bind(server_fd, (struct sockaddr *)&server_addr, server_len) != 0) {
		perror("Bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, QUEUE_SIZE) != 0) {
		perror("Listen");
		exit(EXIT_FAILURE);
	}

	return server_fd;
}

int stats_sv(struct net_data *data, char *buf)
{	
	ssize_t ret;
	size_t len;
	int stats_fd;
	char *buf_tmp;

	printf("Entering function: %s.\n", __func__);

	stats_fd = open("stats", O_RDONLY);
	if (stats_fd < 0) {
		perror("Opening stats file");
		kill(getppid(), SIGTERM);
		exit(ENOENT);
	}

	buf_tmp = buf;
	len = page_size - 1;
	
	while (len != 0 && (ret = read(stats_fd, buf_tmp, len)) != 0) {
		if (ret == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case EINVAL:
				sprintf(buf, "Could not read stats\n");
				goto send;
			default:
				/* Something serious */
				perror("Read");
				exit(EIO);
			}
		}

		len -= ret;
		buf_tmp += ret;
	}
	*buf_tmp = '\0';

send:
	net_data_init(data, LIST_STATS_ID, buf, data->fd);
	send_net_data(data);

	close(stats_fd);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int ladd_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int add_fd;
	char *buf_tmp;

	printf("Entering function: %s.\n", __func__);

	add_fd = open("add", O_WRONLY);
	if (add_fd < 0) {
		perror("Opening add file");
		kill(getppid(), SIGTERM);
		exit(ENOENT);
	}
	
	buf_tmp = data->payload;
	if ((len = data->message_size) == 0) {
		sprintf(buf, "Can't add empty string\n");
		goto send;
	}
	
	/* Write payload to kernel list */
	while (len != 0 && (ret = write(add_fd, buf_tmp, len)) != 0) {
		if (ret == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case EINVAL:
				sprintf(buf, "Could not add element to the list\n");
				goto send;
			default:
				/* Something serious */
				perror("Write");
				exit(EIO);
			}
		}

		/* TODO deal with partial writes */
		len -= ret;
		buf_tmp += ret;
	}

	sprintf(buf, "Added item to list successfully\n");

send:
	net_data_init(data, LIST_ADD_ID, buf, data->fd);
	send_net_data(data);

	close(add_fd);
	
	printf("Exiting function: %s.\n", __func__);
	
	return 0;
}

int ldel_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int del_fd;
	char *buf_tmp;

	printf("Entering function: %s.\n", __func__);
	
	/* Delete list entry */
	del_fd = open("del", O_WRONLY);
	if (del_fd < 0) {
		perror("Opening del file");
		kill(getppid(), SIGTERM); /* Serious error.
					   * Kernel module not loaded 
					   */
		exit(ENOENT);
	}
	
	buf_tmp = data->payload;
	len = data->message_size;

	while (len != 0 && (ret = write(del_fd, buf_tmp, len)) != 0) {
		if (ret == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case EINVAL:
				sprintf(buf, "Element with seq %s not found in the list\n",
					data->payload);
				goto send;
			default:
				/* Something serious */
				perror("Write");
				exit(EIO);
			}
		}
		/* TODO deal with partial writes */
		len -= ret;
		buf_tmp += ret;
	}
		
	
	sprintf(buf, "Deleted element form list\n");

send:
	net_data_init(data, LIST_DEL_ID, buf, data->fd);
	send_net_data(data);

	close(del_fd);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int padd_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int page_fd;
	char *buf_tmp;

	printf("Entering function: %s.\n", __func__);
	
	page_fd = open("page", O_WRONLY);
	if (page_fd < 0) {
		perror("Opening del file");
		kill(getppid(), SIGTERM);
		exit(ENOENT);
	}
	
	buf_tmp = data->payload;
	if ((len = data->message_size) == 0) {
		sprintf(buf, "Can't add empty string\n");
		goto send;
	}
 	
	while (len != 0 && (ret = write(page_fd, buf_tmp, len)) != 0) {
		if (ret == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case EINVAL:
				sprintf(buf, "Could not add content to page\n");
				goto send;
			default:
				/* Something serious */
				perror("Write");
				exit(EIO);
			}
		}
		
		/* TODO deal with partial writes */
		len -= ret;
		buf_tmp += ret;
	}	
	
	sprintf(buf, "Content added successfully\n");

send:
	net_data_init(data, PADD_ID, buf, data->fd);
	send_net_data(data);

	close(page_fd);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int pread_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int page_fd;
	char *buf_tmp;

	printf("Entering function: %s.\n", __func__);

	page_fd = open("page", O_RDONLY);
	if (page_fd < 0) {
		perror("Opening page file");
		kill(getppid(), SIGTERM);
		exit(ENOENT);
	}

	buf_tmp = buf;
	len = page_size - 1;
	
	while (len != 0 && (ret = read(page_fd, buf_tmp, len)) != 0) {
		if (ret == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case EINVAL:
				sprintf(buf, "Error reading content of page buffer\n");
				goto send;
			default:
				/* Something serious */
				perror("Read");
				exit(EIO);
			}
		}

		len -= ret;
		buf_tmp += ret;
	}
	*buf_tmp = '\0';

send:
	net_data_init(data, PREAD_ID, buf, data->fd);
	send_net_data(data);

	close(page_fd);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int ping_sv(struct net_data *data, char *buf)
{
	printf("Entering function %s\n", __func__);

	sprintf(buf, "Server is alive\n");

	net_data_init(data, PING_ID, buf, data->fd);
	send_net_data(data);

	printf("Exiting function %s\n", __func__);
	
	return 0;
}

void server_clean(char *page_buf)
{
	pid_t pid;

	fprintf(stderr, "Waiting for all children to exit\n");
	while ((pid = wait(NULL)) > 0);
	
	free(page_buf);
	
	fprintf(stderr, "Shutting server down\n");
	exit(EXIT_FAILURE);
}

/* Client side */

int stats_cl(struct net_data *data, int sock_fd)
{
	printf("Entering function: %s.\n", __func__);
	
	net_data_init(data, LIST_STATS_ID, NULL, sock_fd);
	send_net_data(data);

	printf("Exiting function: %s.\n", __func__);
	
	return 0;
}

int add_cl(struct net_data *data, char *msg, int sock_fd)
{
	printf("Entering function: %s.\n", __func__);

	net_data_init(data, LIST_ADD_ID, msg, sock_fd);
	send_net_data(data);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int del_cl(struct net_data *data, char *msg, int sock_fd)
{
	printf("Entering function: %s.\n", __func__);

	net_data_init(data, LIST_DEL_ID, msg, sock_fd);
	send_net_data(data);

	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int padd_cl(struct net_data *data, char *msg, int sock_fd)
{		
	printf("Entering function: %s.\n", __func__);

	net_data_init(data, PADD_ID, msg, sock_fd);
	send_net_data(data);
	
	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int pread_cl(struct net_data *data, int sock_fd)
{
	printf("Entering function: %s.\n", __func__);

	net_data_init(data, PREAD_ID, NULL, sock_fd);
	send_net_data(data);

	printf("Exiting function: %s.\n", __func__);

	return 0;
}

int ping_cl(struct net_data *data, int sock_fd)
{		
	printf("Entering function: %s.\n", __func__);

	net_data_init(data, PING_ID, NULL, sock_fd);
	send_net_data(data);

	printf("Exiting function: %s.\n", __func__);

	return 0;
}

/* Handle received messages synchronously */
int get_ans_sync(struct net_data *data)
{
	short timeout;
	fd_set read_fds;
	int ret;
	struct timeval tv;

	timeout = TIMEOUT;
	
	FD_ZERO(&read_fds);
	FD_SET(data->fd, &read_fds);
	
	while (timeout) {
		ret = select(data->fd + 1, &read_fds, NULL, NULL, &tv);
		if (ret == -1) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		
		if (FD_ISSET(data->fd, &read_fds) == 0) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
	
			--timeout;
			printf("%d seconds until timeout.\n", timeout);
		} else {
			/* Received message from server */
			ret = get_net_data(data, data->fd);
			if (ret != 0) {
				switch(ret) {
				case -EIO:
					fprintf(stderr, "Server closed the connection unexpectedly\n");
					break;
				case -EINVAL:
					fprintf(stderr, "Magic number mismatch\n");
					break;
				}
				close(data->fd);
				exit(EXIT_FAILURE);
			}

			/* The server will always send something back */
			printf("%s", data->payload);

			close(data->fd);
			return 0;
		}
	}

	/* Timeout occurred - TODO */
	printf("Timeout\n");
	
	return -1;
}

/* Common */

/* Call with payload = NULL for commands that do not send data */
int net_data_init(struct net_data *data, short command_id, char *payload, int sock_fd)
{
	printf("Entering function: %s.\n", __func__);
	
	data->header = MAGIC_NR;
	data->header |= ((uint32_t)command_id << 16);
	printf("Filled net_data structure with data->header = %08x.\n", data->header);
	data->header = htonl(data->header); /* Host to network endianness */
	
	data->message_size = 0;
	data->payload = payload;
	if (payload != NULL)
		data->message_size = strlen(payload);

	data->fd = sock_fd;
	
	return 0;
}

int send_net_data(struct net_data *data)
{
	uint8_t *buf, *buf_tmp;
	size_t data_len;
	ssize_t ret;
	
	data_len = sizeof(data->header) + sizeof(data->message_size) + data->message_size;
	buf = malloc(data_len);
	if (buf == NULL) {
		fprintf(stderr, "Failed to allocate %lu bytes.\n", data_len);
		exit(EXIT_FAILURE);
	}
	buf_tmp = buf; /* Save this address for later use.
			* Perform pointer arithmetic on buf_tmp. 
			*/

	/* Serialization */
	memcpy(buf_tmp, &data->header, sizeof(data->header));
	buf_tmp += sizeof(data->header);
	
	memcpy(buf_tmp, &data->message_size, sizeof(data->message_size));
	buf_tmp += sizeof(data->message_size);
	
	if (data->message_size > 0)
		memcpy(buf_tmp, data->payload, data->message_size);

	/* Send serialized data through socket */
	buf_tmp = buf; /* Reset to old value */
	while (data_len != 0 && (ret = write(data->fd, buf_tmp, data_len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror ("write");
			break;
		}

		data_len -= ret;
		buf_tmp += ret;
	}

	/* Cleanup */
	free(buf);
	
	return 0;
}

int get_net_data(struct net_data *data, int sock_fd) {
	ssize_t ret;
	size_t len;
	uint32_t *header_buf, *msg_size_buf;
	char *buf;

	printf("Entering function: %s\n", __func__);

	/* Read header*/
	len = sizeof(data->header);
	header_buf = &data->header;
	
	while (len != 0 && (ret = read(sock_fd, header_buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror("read");
			break;
		}

		len -= ret;
		header_buf += ret;
	}
	/* When read returns 0, it means that the socket was closed on the other
	 * side.
	 */
	if (ret == 0)
		return -EIO;
	
	/* Validate header */
	data->header = ntohl(data->header);/* Convert from network to host endianness */
	printf("Got header: %08x \n", data->header); /* DEBUG */
	if (GET_MAGIC(data->header) != MAGIC_NR) {
		fprintf(stderr, "MAGIC NUMBER mismatch\n");
		return -EINVAL;
	}

	/* Read message size */
	len = sizeof(data->message_size);
	msg_size_buf = &data->message_size;
	
	while (len != 0 && (ret = read(sock_fd, msg_size_buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			/* TODO */
			perror("read");
			break;
		}

		printf("Read %ld bytes form message_size\n", ret);
		len -= ret;
		msg_size_buf += ret;
	}
	if (ret == 0)
		return -EIO;
	
	/* Copy socket fd */
	data->fd = sock_fd;

	/* Read payload, if any */
	if (data->message_size == 0)
		return 0;

	len = data->message_size;
	data->payload = malloc(len + 1);
	if (data->payload == NULL) {
		fprintf(stderr, "Error allocationg %lu bytes.\n", len);
		exit(EXIT_FAILURE);
	}
	buf = data->payload;
	
	while (len != 0 && (ret = read(sock_fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			/* TODO */
			perror("read");
			break;
		}
		
		len -= ret;
		buf += ret;
	}
	*buf = '\0';

	if (ret == 0)
		return -EIO;
	
	printf("Exiting function: %s\n", __func__);

	return 0;
}
