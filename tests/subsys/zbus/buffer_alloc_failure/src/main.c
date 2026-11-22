/*
 * Copyright (c) 2026 Jean Nanchen <jean.nanchen@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>

static unsigned int listener_calls;

static void on_message(const struct zbus_channel *chan)
{
	ARG_UNUSED(chan);
	++listener_calls;
}

ZBUS_LISTENER_DEFINE(listener, on_message);
ZBUS_MSG_SUBSCRIBER_DEFINE(msg_subscriber);

NET_BUF_POOL_HEAP_DEFINE(exhausted_pool, 2, sizeof(struct zbus_channel *), NULL);

ZBUS_CHAN_DEFINE(exhausted_pool_chan,                      /* Name */
		 int,                                       /* Message type */
		 NULL,                                      /* Validator */
		 NULL,                                      /* User data */
		 ZBUS_OBSERVERS(listener, msg_subscriber),  /* observers */
		 ZBUS_MSG_INIT(0)                           /* Initial value */
);

ZTEST(buffer_alloc_failure, test_returns_enomem_when_pool_is_exhausted)
{
	const struct zbus_channel *received_channel;
	const int msg = 42;
	int received_msg;
	struct net_buf *first = net_buf_alloc_len(&exhausted_pool, sizeof(int), K_NO_WAIT);
	struct net_buf *second = net_buf_alloc_len(&exhausted_pool, sizeof(int), K_NO_WAIT);

	zassert_not_null(first);
	zassert_not_null(second);
	zbus_chan_set_msg_sub_pool(&exhausted_pool_chan, &exhausted_pool);

	zassert_equal(zbus_chan_pub(&exhausted_pool_chan, &msg, K_NO_WAIT), -ENOMEM);
	zassert_equal(zbus_chan_notify(&exhausted_pool_chan, K_NO_WAIT), -ENOMEM);
	zassert_equal(listener_calls, 0);
	zassert_ok(zbus_chan_read(&exhausted_pool_chan, &received_msg, K_NO_WAIT));
	zassert_equal(received_msg, msg);

	net_buf_unref(first);
	net_buf_unref(second);

	zassert_ok(zbus_chan_notify(&exhausted_pool_chan, K_NO_WAIT));
	zassert_equal(listener_calls, 1);
	zassert_ok(zbus_sub_wait_msg(&msg_subscriber, &received_channel, &received_msg, K_NO_WAIT));
	zassert_equal_ptr(received_channel, &exhausted_pool_chan);
	zassert_equal(received_msg, msg);
}

ZTEST_SUITE(buffer_alloc_failure, NULL, NULL, NULL, NULL, NULL);
