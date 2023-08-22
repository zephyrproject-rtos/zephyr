# Intel ISH family PM default configuration options
#
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if PM

config GDT_DYNAMIC
	default y

config PM_DEVICE
	default y

config GDT_RESERVED_NUM_ENTRIES
	default 3

config REBOOT
	default y

config PM_NEED_ALL_DEVICES_IDLE
	default y

endif
