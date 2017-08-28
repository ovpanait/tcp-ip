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

#include "client.h"
#include "common.h"

struct command commands[] = {
	{"add", LIST_ADD_ID, &add_cl},
	{"del", LIST_DEL_ID, &del_cl},
	{"stats", LIST_STATS_ID, &stats_cl},
	{"padd", PADD_ID, &padd_cl},
	{"pread", PREAD_ID, &pread_cl},
	{"ping", PING_ID, &ping_cl},
	{"exit", 0, &exit_cli},
	{"", 0, NULL}
};

/* Client side */

int stats_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	net_data_init(data, LIST_STATS_ID, NULL, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int add_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif
	net_data_init(data, LIST_ADD_ID, msg, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif
	return 0;
}

int del_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	net_data_init(data, LIST_DEL_ID, msg, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int padd_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	net_data_init(data, PADD_ID, msg, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int pread_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	net_data_init(data, PREAD_ID, NULL, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int ping_cl(struct net_data *data, char *msg, int sock_fd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	net_data_init(data, PING_ID, NULL, sock_fd);
	send_net_data(data);

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return 0;
}

int exit_cli(struct net_data *data, char *msg, int sock_fd)
{
	printf("Exiting CLI......\n");
	exit(EXIT_SUCCESS);
}

/* Handle received messages synchronously */
int get_ans_sync(struct net_data *data)
{
	short timeout;
	fd_set read_fds;
	int ret;
	struct timeval tv;

#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

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

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return -1;
}

/* The buffers are supposed to be at least of size MAX_LEN */
int parse_line(char *cmd, char *arg)
{
	short quote;

	char line[MAX_LINE];
	char *p;

#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	p = fgets(line, MAX_LINE, stdin);
	if (p == NULL) {
		if (ferror(stdin)) {
			perror("fgets");
			exit(EXIT_FAILURE);
		}

		if(feof(stdin))
			return -EINVAL;
	}

	quote = 0;
	*cmd = '\0';
	*arg = '\0';

	/* Skip whitespaces */
	while (*p && *p != '\n' && isspace(*p))
		++p;

	if (*p == '\0' || isspace(*p))
		return -EINVAL;

	/* Fill cmd buffer */
	while (*p && !isspace(*p))
		*cmd++ = *p++;
	*cmd = '\0';

	/* Skip whitespaces */
	while (*p && *p != '\n' && isspace(*p))
		++p;

	if (*p == '\0' || isspace(*p))
		return 0;

	/* Fill arg buffer - Take double quotes into consideration - */
	while (*p && *p != '\n' && (quote || !isspace(*p))) {
		switch (*p) {
		case '\"':
			quote = !quote;
			break;
		default:
			*arg++ = *p;
			break;
		}
		++p;
	}
	*arg = '\0';

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	/* Check for unbalanced double quotes */
	if (quote)
		return -EINVAL;

	return 0;
}

struct command *get_cmdp(char *cmd)
{
#ifdef DEBUG
	printf("Entering function: %s.\n", __func__);
#endif

	struct command *p;

	p = commands;
	while (*(p->name)) {
		if (strcmp(p->name, cmd) == 0)
			return p;
		++p;
	}

#ifdef DEBUG
	printf("Exiting function: %s.\n", __func__);
#endif

	return NULL;
}
