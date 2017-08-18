#include <stdio.h>
#include <stdint.h>
#include "helpers.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>

/* Server side */

int server_init(int *fd_arr)
{
	int ret;
	
	/* Open necessary files */

	ret = chdir("/sys/kernel/debug/ovidiu");
	if (ret == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}
	
	fd_arr[stats_index] = open("stats", O_RDONLY);
	if (fd_arr[stats_index] < 0) {
		fprintf(stderr, "Error opening stats file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	fd_arr[add_index] = open("add", O_WRONLY);
	if (fd_arr[add_index] < 0) {
		fprintf(stderr, "Error opening add file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	
	fd_arr[del_index] = open("del", O_WRONLY);
	if (fd_arr[del_index] < 0) {
		fprintf(stderr, "Error opening del file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	fd_arr[page_index] = open("page", O_RDWR);
	if (fd_arr[page_index] < 0) {
		fprintf(stderr, "Error opening page file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return 0;
}

void server_clean(char *page_buf, int *fd_arr)
{
	short i;
	
	for (i = 0; i < FD_ARR_COUNT; ++i)
		close(fd_arr[i]);

	free(page_buf);
}

/* Client side */

int stats_rq(int sock_fd)
{
	printf("Entering function: %s.\n", __func__);
	
	struct net_data data;
	net_data_init(&data, LIST_STATS_ID, NULL);
	send_net_data(&data, sock_fd);

	return 0;
}

/* Common */

/* Call with payload = NULL for commands that do not send data */
int net_data_init(struct net_data *data, short command_id, char *payload)
{
	printf("Entering function: %s.\n", __func__);
	
	data->header = MAGIC_NR;
	data->header |= ((uint32_t)command_id << 16);
	printf("Filled net_data structure with data->header = %08x.\n", data->header);
	data->header = htonl(data->header); /* Host to network endianness */
	printf("Final data->header: %08x.\n", data->header);
	
	data->message_size = 0;
	data->payload = payload;
	if (payload != NULL)
		data->message_size = strlen(payload);

	return 0;
}


int send_net_data(struct net_data *data, int sock_fd)
{
	uint8_t *buf;
	size_t data_len;
	ssize_t ret;
	
	data_len = sizeof(data->header) + sizeof(data->message_size) + data->message_size;
	buf = malloc(data_len);
	if (buf == NULL) {
		fprintf(stderr, "Failed to allocate %lu bytes.\n", data_len);
		exit(EXIT_FAILURE);
	}
	
	/* Serialization */
	memcpy(buf, &data->header, sizeof(data->header));
	buf += sizeof(data->header);
	
	memcpy(buf, &data->message_size, sizeof(data->message_size));
	buf += sizeof(data->message_size);

	if (data->message_size > 0) {
		memcpy(buf, data->payload, data->message_size);
		buf += data->message_size;
	}
	
	printf("Sending buf : %s", buf);
	
	/* Send serialized data through socket */
	while (data_len != 0 && (ret = write(sock_fd, buf - data_len, data_len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror ("write");
			break;
		}

		printf("Sent %d bytes through socket.\n", ret);
		
		data_len -= ret;
		buf += ret;
	}

	return 0;
}

int get_net_data(struct net_data *data, int sock_fd) {
	ssize_t ret;
	size_t len;
	uint32_t *header_buf, *msg_size_buf;
	char *buf;
	
	len = sizeof(data->header);
	header_buf = &data->header;
	
	/* Read header*/
	while (len != 0 && (ret = read(sock_fd, header_buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror("read");
			break;
		}

		printf("Read %ld bytes of the header.\n", ret);
		len -= ret;
		header_buf += ret;
	}

	/* Validate header */
	data->header = ntohl(data->header);/* Convert from network to host endianness */
	printf("Got header: %08x \n", data->header); /* DEBUG */
	if (GET_MAGIC(data->header) != MAGIC_NR) {
		/* TODO send_failure(sock_fd); */
		return -1;
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
		
		len -= ret;
		msg_size_buf += ret;
	}
	
	/* Read payload, if any */

	if (data->message_size == 0)
		return 0;

	len = data->message_size;
	data->payload = malloc(len);
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
	
	return 0;
}
