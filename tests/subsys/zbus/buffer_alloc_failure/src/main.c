/*
 * Copyright (c) 2026 Jean Nanchen <jean.nanchen@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>

ZBUS_MSG_SUBSCRIBER_DEFINE(msg_subscriber);

NET_BUF_POOL_HEAP_DEFINE(exhausted_pool, 1, sizeof(struct zbus_channel *), NULL);

ZBUS_CHAN_DEFINE(exhausted_pool_chan,                      /* Name */
		 int,                                       /* Message type */
		 NULL,                                      /* Validator */
		 NULL,                                      /* User data */
		 ZBUS_OBSERVERS(msg_subscriber),            /* observers */
		 ZBUS_MSG_INIT(0)                           /* Initial value */
);

ZTEST(buffer_alloc_failure, test_returns_enomem_when_pool_is_exhausted)
{
	struct net_buf *buf = net_buf_alloc_len(&exhausted_pool, sizeof(int), K_NO_WAIT);

	zassert_not_null(buf);
	zbus_chan_set_msg_sub_pool(&exhausted_pool_chan, &exhausted_pool);
	zassert_equal(-ENOMEM, zbus_chan_notify(&exhausted_pool_chan, K_NO_WAIT));

	net_buf_unref(buf);
}

ZTEST_SUITE(buffer_alloc_failure, NULL, NULL, NULL, NULL, NULL);
