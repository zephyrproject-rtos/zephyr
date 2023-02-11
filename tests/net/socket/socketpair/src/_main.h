/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_SOCKETPAIR_THREAD_H_
#define TEST_SOCKETPAIR_THREAD_H_

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_ARCH_POSIX
#include <fcntl.h>
#else
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/unistd.h>
#endif

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

LOG_MODULE_DECLARE(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

extern struct k_work_q test_socketpair_work_q;

#endif /* TEST_SOCKETPAIR_THREAD_H_ */
