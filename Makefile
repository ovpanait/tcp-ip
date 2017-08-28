#
# Compiler flags
#
CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wno-unused-parameter

#
#
#
SV_OBJS = server.o server_fc.o common.o
CL_OBJS = client.o client_fc.o common.o

.PHONY: clean

#
#
#
image: $(SV_OBJS) $(CL_OBJS)
	$(CC) $(CFLAGS) -o server $(SV_OBJS)
	$(CC) $(CFLAGS) -o client $(CL_OBJS)

debug: CFLAGS += -DDEBUG
debug: image

clean:
	-rm server client *.o 2> /dev/null

$(SV_OBJS) : server.h common.c server_fc.c
$(CL_OBJS) : client.h common.c client_fc.c
common.c : common.h
