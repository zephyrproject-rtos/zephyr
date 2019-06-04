/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_FCNTL_H_

#define O_CREAT    0x0200
#define O_EXCL     0x0800
#define O_NONBLOCK 0x4000

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

int open(const char *name, int flags);

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_FCNTL_H_ */
