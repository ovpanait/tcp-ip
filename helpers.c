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

/* Client size */

/* Call with payload = NULL for commands that do not send data */

int net_data_init(struct net_data *data, short command_id, char *payload)
{
	data->header = htons(MAGIC_NR);
	data->header = (htons(command_id) << 16);

	data->message_size = 0;
	data->payload = payload;
	if (payload != NULL)
		data->message_size = strlen(payload);

	return 0;
}

void *serialize(struct net_data *data)
{
	void *ser_ptr;
	
	ser_ptr = malloc(sizeof(data->header) + sizeof(data->message_size) + data->message_size);
	
	memcpy(ser_ptr, &data->header, sizeof(data->header));
	memcpy(ser_ptr, &data->message_size, sizeof(data->message_size));
	if (data->message_size > 0)
		memcpy(ser_ptr, data->payload, data->message_size);

	return ser_ptr;
}

/* Common */
