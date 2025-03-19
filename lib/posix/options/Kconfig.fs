# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

menuconfig POSIX_FILE_SYSTEM
	bool "POSIX file system API support"
	default y if POSIX_API
	select FILE_SYSTEM
	select FDTABLE
	help
	  This enables POSIX style file system related APIs.

if POSIX_FILE_SYSTEM

config POSIX_FILE_SYSTEM_ALIAS_FSTAT
	bool
	help
	  When selected via Kconfig, Zephyr will provide an alias for fstat() as _fstat().

endif # POSIX_FILE_SYSTEM
