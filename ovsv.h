#ifndef OVSV_H
#define OVSV_H

#define FD_ARR_COUNT 4

enum {
	add_index,
	del_index,
	stats_index,
	page_index
};
	
int server_init(int *fd_arr);
void server_clean(char *page_buf, int *fd_arr);

#endif
