/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_TYPES_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_TYPES_H_

struct semihost_poll_in_args {
	long zero;
} __packed;

struct semihost_open_args {
	const char *path;
	long mode;
	long path_len;
} __packed;

struct semihost_close_args {
	long fd;
} __packed;

struct semihost_flen_args {
	long fd;
} __packed;

struct semihost_seek_args {
	long fd;
	long offset;
} __packed;

struct semihost_read_args {
	long fd;
	char *buf;
	long len;
} __packed;

struct semihost_write_args {
	long fd;
	const char *buf;
	long len;
} __packed;

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_SEMIHOST_TYPES_H_ */
