/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

static ZTEST_DMEM struct net_socketpair_fixture fixture;
static void *setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	return &fixture;
}

static void before(void *arg)
{
	struct net_socketpair_fixture *fixture = arg;

	for (int i = 0; i < 2; ++i) {
		if (fixture->sv[i] >= 0) {
			fixture->sv[i] = -1;
		}
	}
	zassert_ok(zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, fixture->sv));
}

static void after(void *arg)
{
	struct net_socketpair_fixture *fixture = arg;

	for (int i = 0; i < 2; ++i) {
		if (fixture->sv[i] >= 0) {
			zassert_ok(zsock_close(fixture->sv[i]));
			fixture->sv[i] = -1;
		}
	}
}

ZTEST_SUITE(net_socketpair, NULL, setup, before, after, NULL);
