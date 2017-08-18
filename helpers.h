#ifndef HELPERS_H
#define HELPERS_H

#include <sys/types.h>

#define QUEUE_SIZE 5

#define MAGIC_NR 0x123
#define GET_MAGIC(x) \
	((x) & ((1 << 16) - 1))
#define GET_CMD_ID(x) \
	((x) >> 16)

#define LIST_ADD_ID 0x1
#define LIST_STATS_ID 0x2
#define LIST_DEL_ID 0x3
#define PADD_ID 0x4
#define PREAD_ID 0x5
#define PING_ID 0x6

#define FD_ARR_COUNT 4

enum {
	add_index,
	del_index,
	stats_index,
	page_index
};
	
struct net_data {
	uint32_t header;
	uint32_t message_size;
	char *payload;
};

/* Prototypes */
int server_init(int *fd_arr);
void server_clean(char *page_buf, int *fd_arr);
int net_data_init(struct net_data *data, short command_id, char *payload);
int send_net_data(struct net_data *data, int sock_fd);
int stats_rq(int sock_fd);
int get_net_data(struct net_data *data, int sock_fd);

#endif
