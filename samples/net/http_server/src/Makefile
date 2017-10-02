#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

include $(ZEPHYR_BASE)/samples/net/common/Makefile.common

# List of files that are used to generate .h file that can be included
# into .c file.
generate_inc_file += \
	echo-apps-cert.der \
	echo-apps-key.der

generate_inc_file += \
	index.html

include $(ZEPHYR_BASE)/scripts/Makefile.gen

obj-y += main.o
