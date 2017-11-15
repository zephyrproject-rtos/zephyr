#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

# List of files that are used to generate .h file that can be included
# into .c file.
generate_inc_file += \
	echo-apps-cert.der \
	echo-apps-key.der

generate_inc_file += \
	index.html

generate_inc_gz_file += \
	ws.js

include $(ZEPHYR_BASE)/scripts/Makefile.gen

include $(ZEPHYR_BASE)/samples/net/common/Makefile.common

obj-y += main.o
obj-y += ws.o
