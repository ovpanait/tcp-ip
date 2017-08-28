#
# Compiler flags
#
CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wno-unused-parameter

#
#
#
SV_OBJS = ovsv.o helpers.o
CL_OBJS = ovcl.o helpers.o

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

$(SV_OBJS) $(CL_OBJS): helpers.c
helpers.c : helpers.h
