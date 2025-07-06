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
#include <zephyr/posix/fcntl.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#if !defined(_main_defined)
LOG_MODULE_DECLARE(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);
#endif

struct net_socketpair_fixture {
	int sv[2];
};

#endif /* TEST_SOCKETPAIR_THREAD_H_ */
