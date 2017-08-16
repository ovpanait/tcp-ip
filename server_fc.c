#include <stdio.h>
#include "ovsv.h"
#include <unistd.h>

char *server_init(int *fd_arr, int buf_size)
{
	/* Open necessary files */

	
	char *page_buf;

 	page_buf = malloc(buf_size);
	if (page_buf == NULL) {
		fprintf(stderr, "Failed to allocate %d bytes.\n", page_size);
		exit(EXIT_FAILURE);
	}
	
	return page_buf;
}

void server_clean(char *page_buf, int *fd_arr)
{
	short i;
	
	for (i = 0; i < FD_ARR_COUNT; ++i)
		close(fd_arr[i]);

	free(page_buf);
}
