/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct msg {
	int x;
};

ZBUS_CHAN_DEFINE(chan_a, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_b, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_c, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_d, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_e, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_f, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

ZTEST(channel_name, test_channel_retrieval)
{
	/* Invalid/unknown channel names */
	zassert_is_null(zbus_chan_from_name("unknown"));
	zassert_is_null(zbus_chan_from_name(""));

	/* Standard retrieval */
	zassert_equal(&chan_a, zbus_chan_from_name("chan_a"));
	zassert_equal(&chan_b, zbus_chan_from_name("chan_b"));
	zassert_equal(&chan_c, zbus_chan_from_name("chan_c"));

	/* Ensure no cross-talk between names */
	zassert_not_equal(&chan_d, zbus_chan_from_name("chan_e"));
	zassert_not_equal(&chan_e, zbus_chan_from_name("chan_f"));
	zassert_not_equal(&chan_f, zbus_chan_from_name("chan_d"));
}

ZTEST_SUITE(channel_name, NULL, NULL, NULL, NULL, NULL);
