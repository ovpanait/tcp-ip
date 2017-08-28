#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

#define MAGIC_NR 0x123
/* Retrieve magic number form header */
#define GET_MAGIC(x) \
	((x) & ((1 << 16) - 1))
/* Retrieve command id from header */
#define GET_CMD_ID(x) \
	((x) >> 16)

#define LIST_ADD_ID 0x1
#define LIST_STATS_ID 0x2
#define LIST_DEL_ID 0x3
#define PADD_ID 0x4
#define PREAD_ID 0x5
#define PING_ID 0x6

#define QUEUE_SIZE 5
#define TIMEOUT 5

#define SRV_ADDR "127.0.0.1"
#define PORT 9734

#define MAX_LINE 8096

struct net_data {
	uint32_t header;
	uint32_t message_size;
	char *payload;

	int fd; /* The file descriptor the message is bound to */
};

extern int page_size;

int net_data_init(struct net_data *data, short command_id, char *payload, int sock_fd);
int send_net_data(struct net_data *data);
int get_net_data(struct net_data *data, int sock_fd);

#endif
