CFLAGS_klist.o := -DDEBUG

obj-m := klist.o

SRC_DIR ?= /usr/src/linux-headers-$(shell uname -r)

build_modules:
	make -C $(SRC_DIR) SUBDIRS=${CURDIR} modules

clean:
	make -C $(SRC_DIR) M=${CURDIR} clean
