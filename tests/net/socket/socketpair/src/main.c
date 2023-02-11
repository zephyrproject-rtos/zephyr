/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

static void *setup(void)
{
	k_thread_system_pool_assign(k_current_get());
	return NULL;
}

ZTEST_SUITE(net_socketpair, NULL, setup, NULL, NULL, NULL);
