#ifndef HELPERS_H
#define HELPERS_H

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

#define FD_ARR_COUNT 4
#define QUEUE_SIZE 5
#define TIMEOUT 5
	
struct net_data {
	uint32_t header;
	uint32_t message_size;
	char *payload;

	int fd; /* The file descriptor the message is bound to */
};

extern int page_size;

/* Prototypes */
int server_init(void);
void server_clean(char *page_buf);

int net_data_init(struct net_data *data, short command_id, char *payload, int sock_fd);
int send_net_data(struct net_data *data);
int get_net_data(struct net_data *data, int sock_fd);

int stats_rq(struct net_data *data, int sock_fd);
int add_rq(struct net_data *data, char *msg, int sock_fd);
int ping_rq(struct net_data *data, int sock_fd);
int pread_rq(struct net_data *data, int sock_fd);
int padd_rq(struct net_data *data, char *msg, int sock_fd);
int del_rq(struct net_data *data, char *msg, int sockf_fd);

int stats_send(struct net_data *data, char *buf);
int ladd_send(struct net_data *data, char *buf);

int get_sync_answer(struct net_data *data);

#endif
