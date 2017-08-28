#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

struct command {
	char name[7];
	uint16_t id;
	int (*fc) (struct net_data *, char *, int);
};

extern struct command commands[];

int stats_cl(struct net_data *data, char *msg, int sock_fd);
int add_cl(struct net_data *data, char *msg, int sock_fd);
int ping_cl(struct net_data *data, char *msg, int sock_fd);
int pread_cl(struct net_data *data, char *msg,  int sock_fd);
int padd_cl(struct net_data *data, char *msg, int sock_fd);
int del_cl(struct net_data *data, char *msg, int sockf_fd);
int exit_cli(struct net_data *data, char *msg, int sockf_fd);

int get_ans_sync(struct net_data *data);

int parse_line(char *cmd, char *arg);
struct command *get_cmdp(char *cmd);

#endif
