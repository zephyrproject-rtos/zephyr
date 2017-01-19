/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>
#include <sections.h>

#include <tc_util.h>

#include <net/buf.h>

#include "icmpv6.h"

static int handler_called;

struct header {
	int status;
};

#define buf_status(buf) (((struct header *)net_buf_user_data((buf)))->status)

#define TEST_MSG "foobar devnull"

NET_BUF_POOL_DEFINE(bufs_pool, 2, 0, sizeof(struct header), NULL);

NET_BUF_POOL_DEFINE(data_pool, 2, 128, 0, NULL);

static enum net_verdict handle_test_msg(struct net_buf *buf)
{
	struct net_buf *last = net_buf_frag_last(buf);

	if (last->len != (strlen(TEST_MSG) + 1)) {
		buf_status(buf) = -EINVAL;
	} else {
		buf_status(buf) = 0;
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

static bool run_tests(void)
{
	struct net_buf *buf, *frag;
	int ret;

	net_icmpv6_register_handler(&test_handler1);
	net_icmpv6_register_handler(&test_handler2);

	buf = net_buf_alloc(&bufs_pool, K_FOREVER);
	frag = net_buf_alloc(&data_pool, K_FOREVER);

	net_buf_frag_add(buf, frag);

	memcpy(net_buf_add(frag, sizeof(TEST_MSG)),
	       TEST_MSG, sizeof(TEST_MSG));

	ret = net_icmpv6_input(buf, net_buf_frags_len(buf->frags), 0, 0);
	if (!ret) {
		printk("%d: Callback not called properly\n", __LINE__);
		return false;
	}

	ret = net_icmpv6_input(buf, net_buf_frags_len(buf->frags),
			       NET_ICMPV6_ECHO_REPLY, 0);
	if (ret < 0 || buf_status(buf) != 0) {
		printk("%d: Callback not called properly\n", __LINE__);
		return false;
	}

	ret = net_icmpv6_input(buf, net_buf_frags_len(buf->frags), 1, 0);
	if (!ret) {
		printk("%d: Callback not called properly\n", __LINE__);
		return false;
	}

	ret = net_icmpv6_input(buf, net_buf_frags_len(buf->frags),
			       NET_ICMPV6_ECHO_REQUEST, 0);
	if (ret < 0 || buf_status(buf) != 0) {
		printk("%d: Callback not called properly\n", __LINE__);
		return false;
	}

	if (handler_called != 2) {
		printk("%d: Callbacks not called properly\n", __LINE__);
		return false;
	}

	printk("ICMPv6 tests passed\n");

	return true;
}

void main_thread(void)
{
	if (run_tests()) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}

#define STACKSIZE 2000
char __noinit __stack thread_stack[STACKSIZE];

void main(void)
{
	k_thread_spawn(&thread_stack[0], STACKSIZE,
		       (k_thread_entry_t)main_thread, NULL, NULL, NULL,
		       K_PRIO_COOP(7), 0, 0);
}
