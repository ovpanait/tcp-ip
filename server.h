#ifndef SERVER_H
#define SERVER_H

#include "common.h"

int server_init(void);
void server_clean(char *page_buf);

int stats_sv(struct net_data *data, char *buf);
int ladd_sv(struct net_data *data, char *buf);
int ldel_sv(struct net_data *data, char *buf);
int padd_sv(struct net_data *data, char *buf);
int pread_sv(struct net_data *data, char *buf);
int ping_sv(struct net_data *data, char *buf);

#endif
