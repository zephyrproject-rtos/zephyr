#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

PREFIX ?= .
OBJ_DIR ?= $(PREFIX)/obj
LIB_DIR ?= $(PREFIX)/lib

ifeq ($(findstring Windows, $(OS)),Windows)
	DEL := rmdir /S /Q
	MKDIR := mkdir
else
	DEL := rm -rf
	MKDIR := mkdir -p
endif

all:
	-$(MKDIR) "$(OBJ_DIR)"
	-$(MKDIR) "$(LIB_DIR)"
	$(CC) -c $(CFLAGS) -MD -Iinclude src/mylib.c -o $(OBJ_DIR)/mylib.o
	$(AR) -rcs $(LIB_DIR)/libmylib.a $(OBJ_DIR)/mylib.o

clean:
	$(DEL) "$(OBJ_DIR)" "$(LIB_DIR)"
