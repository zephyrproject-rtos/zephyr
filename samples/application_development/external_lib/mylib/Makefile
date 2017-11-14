#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

PREFIX ?= .
OBJ_DIR ?= $(PREFIX)/obj
LIB_DIR ?= $(PREFIX)/lib

all:
	mkdir -p $(OBJ_DIR) $(LIB_DIR)
	$(CC) -c $(CFLAGS) -Iinclude src/mylib.c -o $(OBJ_DIR)/mylib.o
	$(AR) -rcs $(LIB_DIR)/libmylib.a $(OBJ_DIR)/mylib.o

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
