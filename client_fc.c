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

/* All functions that send requests to the server share the same format.
 * Use a macro.
 */
#define REQUEST_FC(name, id)						\
	int name##_cl (struct net_data *data, char *msg)		\
	{								\
		int result;						\
		int sock_fd;						\
									\
		DEBUG_PRINT("Entering function: %s\n", __func__);	\
									\
		sock_fd = socket(AF_INET, SOCK_STREAM, 0);		\
		result = connect(sock_fd, (struct sockaddr *)&address, len); \
		if(result == -1) {					\
			perror("connect");				\
			exit(EXIT_FAILURE);				\
		}							\
									\
		net_data_init(data, id, msg, sock_fd);			\
		send_net_data(data);					\
									\
		DEBUG_PRINT("Exiting function: %s\n", __func__);	\
		return 0;						\
	}

REQUEST_FC(stats, LIST_STATS_ID)
REQUEST_FC(add, LIST_ADD_ID)
REQUEST_FC(del, LIST_DEL_ID)
REQUEST_FC(padd, PADD_ID)
REQUEST_FC(pread, PREAD_ID)
REQUEST_FC(ping, PING_ID)

/* Close command line interface */
int exit_cli(struct net_data *data, char *msg)
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

	DEBUG_PRINT("Entering function %s\n", __func__);

	timeout = TIMEOUT;

	while (timeout) {
		FD_ZERO(&read_fds);
		FD_SET(data->fd, &read_fds);

		ret = select(FD_SETSIZE, &read_fds, NULL, NULL, &tv);
		if (ret == -1) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(data->fd, &read_fds) == 0) {
			tv.tv_sec = 5;
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

	DEBUG_PRINT("Exiting function %s\n", __func__);

	return -1;
}

/* The buffers are supposed to be at least of size MAX_LEN */
int parse_line(char *cmd, char *arg)
{
	short quote;

	char line[MAX_LINE];
	char *p;

	DEBUG_PRINT("Entering function %s\n", __func__);

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

	DEBUG_PRINT("Exiting function %s\n", __func__);

	/* Check for unbalanced double quotes */
	if (quote)
		return -EINVAL;

	return 0;
}

struct command *get_cmdp(char *cmd)
{
	DEBUG_PRINT("Entering function %s\n", __func__);

	struct command *p;

	p = commands;
	while (*(p->name)) {
		if (strcmp(p->name, cmd) == 0)
			return p;
		++p;
	}

	DEBUG_PRINT("Exiting function %s\n", __func__);

	return NULL;
}
