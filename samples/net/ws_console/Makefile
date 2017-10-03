#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD ?= qemu_x86
CONF_FILE ?= prj.conf

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(CONFIG_NET_L2_BT), y)
	QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr
else
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif

all: generate_inc_files generate_inc_gz_files
flash: generate_inc_files generate_inc_gz_files
run: generate_inc_files generate_inc_gz_files

SRC_DIR = $(ZEPHYR_BASE)/samples/net/ws_console/src
generate_inc_files:
	$(MAKE) -C $(SRC_DIR) generate_inc_files Q=$(Q)
generate_inc_gz_files:
	$(MAKE) -C $(SRC_DIR) generate_inc_gz_files Q=$(Q)
