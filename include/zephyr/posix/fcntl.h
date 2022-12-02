/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_POSIX_FCNTL_H_
#define ZEPHYR_POSIX_FCNTL_H_

#ifdef CONFIG_PICOLIBC
#define O_CREAT 0x0040
#else
#define O_CREAT 0x0200
#endif

#define O_APPEND   0x0400
#define O_EXCL	   0x0800
#define O_NONBLOCK 0x4000

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

int open(const char *name, int flags, ...);
int fcntl(int fildes, int cmd, ...);

#endif /* ZEPHYR_POSIX_FCNTL_H_ */
