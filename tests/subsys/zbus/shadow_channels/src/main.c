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

enum channel_ids {
	CHAN_B = 123,
	CHAN_D = 125,
};

/* Normal channels */
ZBUS_CHAN_DEFINE(chan_a, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE_WITH_ID(chan_b, CHAN_B, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));

/* Shadow channels */
ZBUS_SHADOW_CHAN_DEFINE(chan_c, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_SHADOW_CHAN_DEFINE_WITH_ID(chan_d, CHAN_D, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
				ZBUS_MSG_INIT(0));

/* Multidomain channels */
ZBUS_MULTIDOMAIN_CHAN_DEFINE(chan_e, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0),
			     true, true);
ZBUS_MULTIDOMAIN_CHAN_DEFINE(chan_f, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0),
			     false, true);
ZBUS_MULTIDOMAIN_CHAN_DEFINE(chan_g, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0),
			     false, false);

ZTEST(shadow_channels, test_shadow_channel_identification)
{
	/* Check shadow channel identification */
	zassert_false(ZBUS_CHANNEL_IS_SHADOW(&chan_a));
	zassert_false(ZBUS_CHANNEL_IS_SHADOW(&chan_b));

	zassert_true(ZBUS_CHANNEL_IS_SHADOW(&chan_c));
	zassert_true(ZBUS_CHANNEL_IS_SHADOW(&chan_d));

	zassert_false(ZBUS_CHANNEL_IS_SHADOW(&chan_e));
	zassert_true(ZBUS_CHANNEL_IS_SHADOW(&chan_f));

	/* Check master channel identification */
	zassert_true(ZBUS_CHANNEL_IS_MASTER(&chan_a));
	zassert_true(ZBUS_CHANNEL_IS_MASTER(&chan_b));

	zassert_false(ZBUS_CHANNEL_IS_MASTER(&chan_c));
	zassert_false(ZBUS_CHANNEL_IS_MASTER(&chan_d));

	zassert_true(ZBUS_CHANNEL_IS_MASTER(&chan_e));
	zassert_false(ZBUS_CHANNEL_IS_MASTER(&chan_f));
}

ZTEST(shadow_channels, test_shadow_channel_exclusion)
{
	/* NOTE: chan_g should not be defined as _is_included in the macro is false
	 * Therefore, zbus_chan_from_name("chan_g") should return NULL
	 */
	zassert_is_null(zbus_chan_from_name("chan_g"));
}

ZTEST(shadow_channels, test_pub)
{
	struct msg msg = {42};

	/* normal publish cannot be used on shadow channels */
	zassert_equal(-EPERM, zbus_chan_pub(&chan_c, &msg, K_NO_WAIT));
	zassert_equal(-EPERM, zbus_chan_pub(&chan_d, &msg, K_NO_WAIT));
	zassert_equal(-EPERM, zbus_chan_pub(&chan_f, &msg, K_NO_WAIT));

	/* shadow publish can be used on shadow channels */
	zassert_equal(0, zbus_chan_pub_shadow(&chan_c, &msg, K_NO_WAIT));
	zassert_equal(0, zbus_chan_pub_shadow(&chan_d, &msg, K_NO_WAIT));
	zassert_equal(0, zbus_chan_pub_shadow(&chan_f, &msg, K_NO_WAIT));

	/* shadow publish cannot be used on normal channels */
	zassert_equal(-EPERM, zbus_chan_pub_shadow(&chan_a, NULL, K_NO_WAIT));
	zassert_equal(-EPERM, zbus_chan_pub_shadow(&chan_b, NULL, K_NO_WAIT));
	zassert_equal(-EPERM, zbus_chan_pub_shadow(&chan_e, NULL, K_NO_WAIT));
}

ZTEST_SUITE(shadow_channels, NULL, NULL, NULL, NULL, NULL);
