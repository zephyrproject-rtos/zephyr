/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_

#include <sys/types.h>
#include <net/socket.h>

#define SHUT_RD ZSOCK_SHUT_RD
#define SHUT_WR ZSOCK_SHUT_WR
#define SHUT_RDWR ZSOCK_SHUT_RDWR

#define MSG_PEEK ZSOCK_MSG_PEEK
#define MSG_TRUNC ZSOCK_MSG_TRUNC
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT
#define MSG_WAITALL ZSOCK_MSG_WAITALL

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_ */
