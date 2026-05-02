/* main.c - Application main entry point */

/* We are just testing that this program compiles ok with all possible
 * network related Kconfig options enabled.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_DBG);

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/offloaded_netdev.h>


/* Create blank dummy and offloaded APIs */
static struct offloaded_if_api offload_dev_api;
static const struct dummy_api dummy_dev_api;
static struct offload_context {
	void *none;
} offload_context_data = {
	.none = NULL
};

/* Create blank dummy and offloaded net devices */
NET_DEVICE_INIT(dummy_dev, "dummy_dev",
		NULL, NULL,
		NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&dummy_dev_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 0);

NET_DEVICE_OFFLOAD_INIT(net_offload, "net_offload",
			NULL, NULL,
			&offload_context_data, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&offload_dev_api, 0);

ZTEST(net_compile_all_test, test_ok)
{
	zassert_true(true, "This test should never fail");
}

ZTEST_SUITE(net_compile_all_test, NULL, NULL, NULL, NULL, NULL);
