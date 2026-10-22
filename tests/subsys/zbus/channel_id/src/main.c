/*
 * Copyright (c) 2024 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct msg {
	int x;
};

enum channel_ids {
	CHAN_A = 100,
	CHAN_B = 123,
	CHAN_C = 0x12343243,
	CHAN_D = 123,
	CHAN_E = 1,
	CHAN_F = 357489,
};

ZBUS_CHAN_DEFINE_WITH_ID(chan_a, CHAN_A, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_b, CHAN_B, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_c, CHAN_C, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_d, CHAN_D, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_e, CHAN_E, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_f, CHAN_F, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_g, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

ZTEST(channel_id, test_channel_retrieval)
{
	/* Invalid/unknown channel IDs */
	zassert_is_null(zbus_chan_from_id(0x1000000));
	zassert_is_null(zbus_chan_from_id(ZBUS_CHAN_ID_INVALID));

	/* Standard retrieval */
	zassert_equal(&chan_a, zbus_chan_from_id(CHAN_A));
	zassert_equal(&chan_c, zbus_chan_from_id(CHAN_C));
	zassert_equal(&chan_e, zbus_chan_from_id(CHAN_E));
	zassert_equal(&chan_f, zbus_chan_from_id(CHAN_F));

	/* Duplicate channel IDs */
	zassert_true((&chan_b == zbus_chan_from_id(CHAN_B)) ||
		     (&chan_d == zbus_chan_from_id(CHAN_B)));
}

ZTEST_SUITE(channel_id, NULL, NULL, NULL, NULL, NULL);
