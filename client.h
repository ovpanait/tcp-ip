#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

extern int len;
extern struct sockaddr_in address;

#define NO_ARG 0
#define ARG 1

struct command {
	char name[7];
	uint16_t id;
	short arg_f;
	int (*fc) (struct net_data *, char *);
};

extern struct command commands[];

int stats_cl(struct net_data *data, char *msg);
int add_cl(struct net_data *data, char *msg);
int ping_cl(struct net_data *data, char *msg);
int pread_cl(struct net_data *data, char *msg);
int padd_cl(struct net_data *data, char *msg);
int del_cl(struct net_data *data, char *msg);
int exit_cli(struct net_data *data, char *msg);

int get_ans_sync(struct net_data *data);

int parse_line(char *cmd, char *arg);
struct command *get_cmdp(char *cmd, char *arg);

#endif
