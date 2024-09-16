/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_POSIX_FCNTL_H_
#define ZEPHYR_POSIX_FCNTL_H_

#ifdef CONFIG_PICOLIBC
#define O_CREAT	 0x0040
#define O_TRUNC	 0x0200
#define O_APPEND 0x0400
#else
#define O_CREAT	 0x0200
#define O_TRUNC	 0x0400
#define O_APPEND 0x0008
#endif

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR	 02

#define O_EXCL	   0x0800
#define O_NONBLOCK 0x4000

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

#ifdef __cplusplus
extern "C" {
#endif

int open(const char *name, int flags, ...);
int fcntl(int fildes, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_POSIX_FCNTL_H_ */
