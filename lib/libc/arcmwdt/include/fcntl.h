/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_

#define O_CREAT    0x0200
#define O_APPEND   0x0400
#define O_EXCL     0x0800
#define O_NONBLOCK 0x4000

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

#include_next <fcntl.h>

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_ */
