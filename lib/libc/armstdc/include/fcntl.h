/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_FCNTL_H_

#include <zephyr/sys/fdtable.h>

#define O_CREAT    0x0200
#define O_APPEND   0x0400
#define O_EXCL     0x0800
#define O_NONBLOCK ZVFS_O_NONBLOCK

#define F_DUPFD 0
#define F_GETFL ZVFS_F_GETFL
#define F_SETFL ZVFS_F_SETFL

int open(const char *name, int flags, ...);

#endif /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_FCNTL_H_ */
