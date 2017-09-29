#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

all:
	mkdir -p obj lib
	$(CC) -c $(CFLAGS) -Iinclude src/mylib.c -o obj/mylib.o
	$(AR) -rcs lib/libmylib.a obj/mylib.o

clean:
	rm -rf obj lib

