/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

static struct nano_fifo bufs_fifo;
static struct nano_fifo data_fifo;

static NET_BUF_POOL(bufs_pool, 2, 0, &bufs_fifo, NULL, sizeof(struct header));
static NET_BUF_POOL(data_pool, 2, 128, &data_fifo, NULL, 0);

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

	net_buf_pool_init(bufs_pool);
	net_buf_pool_init(data_pool);

	net_icmpv6_register_handler(&test_handler1);
	net_icmpv6_register_handler(&test_handler2);

	buf = net_buf_get(&bufs_fifo, 0);
	frag = net_buf_get(&data_fifo, 0);

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

void main_fiber(void)
{
	if (run_tests()) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void main(void)
{
#if defined(CONFIG_MICROKERNEL)
	main_fiber();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)main_fiber, 0, 0, 7, 0);
#endif
}
