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

config POSIX_MAX_OPEN_FILES
	int "Maximum number of open file descriptors"
	default 16
	help
	  Maximum number of open files. Note that this setting
	  is additionally bounded by CONFIG_POSIX_MAX_FDS.

config POSIX_FSYNC
	bool "Support for fsync()"
	default y
	help
	  This enables fsync() support.

endif # POSIX_FILE_SYSTEM
