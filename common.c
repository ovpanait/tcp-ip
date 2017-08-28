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

int page_size;

/* Common */

/* Call with payload = NULL for commands that do not send data */
int net_data_init(struct net_data *data, short command_id, char *payload, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	data->header = MAGIC_NR;
	data->header |= ((uint32_t)command_id << 16);
	data->header = htonl(data->header); /* Host to network endianness */

	data->message_size = 0;
	data->payload = payload;
	if (payload != NULL)
		data->message_size = strlen(payload);

	data->fd = sock_fd;

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int send_net_data(struct net_data *data)
{
	uint8_t *buf, *buf_tmp;
	size_t data_len;
	ssize_t ret;

#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

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

#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

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

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}
