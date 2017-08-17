#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

#define QUEUE_SIZE 5

#define MAGIC_NR 0x123

#define LIST_ADD_ID 0x1
#define LIST_STATS_ID 0x2
#define LIST_DEL_ID 0x3
#define PADD_ID 0x4
#define PREAD_ID 0x5
#define PING_ID 0x6

struct net_data {
	uint32_t header;
	uint32_t message_size;
	char *payload;
};
	
#endif 
