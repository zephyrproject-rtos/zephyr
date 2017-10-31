/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <net/buf.h>

#include "icmpv6.h"
#include <ztest.h>

static int handler_called;
static int handler_status;

#define TEST_MSG "foobar devnull"

NET_PKT_TX_SLAB_DEFINE(pkts_slab, 2);
NET_BUF_POOL_DEFINE(data_pool, 2, 128, 0, NULL);

static enum net_verdict handle_test_msg(struct net_pkt *pkt)
{
	struct net_buf *last = net_buf_frag_last(pkt->frags);

	if (last->len != (strlen(TEST_MSG) + 1)) {
		handler_status = -EINVAL;
	} else {
		handler_status = 0;
	}

	handler_called++;

	return 0;
}

static struct net_icmpv6_handler test_handler1 = {
	.type = NET_ICMPV6_ECHO_REPLY,
	.code = 0,
	.handler = handle_test_msg,
};

static struct net_icmpv6_handler test_handler2 = {
	.type = NET_ICMPV6_ECHO_REQUEST,
	.code = 0,
	.handler = handle_test_msg,
};

void run_tests(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));

	struct net_pkt *pkt;
	struct net_buf *frag;
	int ret;

	net_icmpv6_register_handler(&test_handler1);
	net_icmpv6_register_handler(&test_handler2);

	pkt = net_pkt_get_reserve(&pkts_slab, 0, K_FOREVER);
	zassert_true(pkt != NULL, "Could get net_pkt from slab");

	frag = net_buf_alloc(&data_pool, K_FOREVER);
	zassert_true(frag != NULL, "Could not allocate buffer from pool");

	net_pkt_frag_add(pkt, frag);

	memcpy(net_buf_add(frag, sizeof(TEST_MSG)),
	       TEST_MSG, sizeof(TEST_MSG));

	ret = net_icmpv6_input(pkt, 0, 0);

	/**TESTPOINT: Check input*/
	zassert_true(ret, "Callback not called properly");

	handler_status = -1;

	ret = net_icmpv6_input(pkt, NET_ICMPV6_ECHO_REPLY, 0);

	/**TESTPOINT: Check input*/
	zassert_true(!(ret < 0 || handler_status != 0),
			"Callback not called properly");

	ret = net_icmpv6_input(pkt, 1, 0);

	/**TESTPOINT: Check input*/
	zassert_true(ret, "Callback not called properly");

	handler_status = -1;

	ret = net_icmpv6_input(pkt, NET_ICMPV6_ECHO_REQUEST, 0);

	/**TESTPOINT: Check input*/
	zassert_true(!(ret < 0 || handler_status != 0),
			"Callback not called properly");

	/**TESTPOINT: Check input*/
	zassert_true(!(handler_called != 2), "Callbacks not called properly");
}

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_icmpv6_fn,
		ztest_unit_test(run_tests));
	ztest_run_test_suite(test_icmpv6_fn);
}
