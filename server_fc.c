#include <stdio.h>
#include <ctype.h>
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

#include "common.h"
#include "server.h"

int server_init(void)
{
	int ret;
	int server_fd;
	socklen_t server_len;
	struct sockaddr_in server_addr;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif

	return server_fd;
}

int stats_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int stats_fd;
	char *buf_tmp;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif
	return 0;
}

int ladd_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int add_fd;
	char *buf_tmp;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif
	return 0;
}

int ldel_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int del_fd;
	char *buf_tmp;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif
	return 0;
}

int padd_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int page_fd;
	char *buf_tmp;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif

	return 0;
}

int pread_sv(struct net_data *data, char *buf)
{
	ssize_t ret;
	size_t len;
	int page_fd;
	char *buf_tmp;

	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

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

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif

	return 0;
}

int ping_sv(struct net_data *data, char *buf)
{
	#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
	#endif

	sprintf(buf, "Server is alive\n");

	net_data_init(data, PING_ID, buf, data->fd);
	send_net_data(data);

	#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
	#endif

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
