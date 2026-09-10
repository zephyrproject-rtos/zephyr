/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/sys/util_macro.h>

struct msg {
	int x;
};

#define CHANNEL_COUNT 10

#define DEFINE_TEST_CHANNELS(i, _)                                                                 \
	ZBUS_CHAN_DEFINE(test_chan_##i, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,              \
			 ZBUS_MSG_INIT(0));

LISTIFY(CHANNEL_COUNT, DEFINE_TEST_CHANNELS, (;))

ZTEST(channel_name, test_channel_retrieval)
{
	/* invalid cases */
	zexpect_is_null(zbus_chan_from_name("unknown"));
	zexpect_is_null(zbus_chan_from_name(NULL));
	zexpect_is_null(zbus_chan_from_name(""));

	/* valid cases */
	zexpect_equal_ptr(&test_chan_0, zbus_chan_from_name("test_chan_0"));
	zexpect_equal_ptr(&test_chan_4, zbus_chan_from_name("test_chan_4"));
	zexpect_equal_ptr(&test_chan_5, zbus_chan_from_name("test_chan_5"));
	zexpect_equal_ptr(&test_chan_9, zbus_chan_from_name("test_chan_9"));
}

ZTEST_SUITE(channel_name, NULL, NULL, NULL, NULL, NULL);
