#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD = qemu_x86
CONF_FILE = prj.conf

KBUILD_ZEPHYR_APP = libmylib.a
SOURCE_DIR = $(CURDIR)
subdir-ccflags-y += -I$(SOURCE_DIR)/mylib/include
obj-y += src/

export obj-y subdir-ccflags-y
export KBUILD_ZEPHYR_APP

include ${ZEPHYR_BASE}/Makefile.inc
