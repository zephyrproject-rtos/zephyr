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

ZBUS_CHAN_DEFINE(chan, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

ZTEST(publish_stats, test_channel_metadata)
{
	k_ticks_t clock_window = CONFIG_SYS_CLOCK_TICKS_PER_SEC / 20; /* Accept +- 50ms */
	struct msg *cval, val = {0};
	k_ticks_t pub_time;

	/* Application boot, no publishes */
	zassert_equal(0, zbus_chan_pub_stats_count(&chan));
	zassert_equal(0, zbus_chan_pub_stats_last_time(&chan));
	zassert_equal(0, zbus_chan_pub_stats_avg_period(&chan));

	/* Should be no different after a second of runtime */
	k_sleep(K_SECONDS(1));
	zassert_equal(0, zbus_chan_pub_stats_count(&chan));
	zassert_equal(0, zbus_chan_pub_stats_last_time(&chan));
	zassert_equal(0, zbus_chan_pub_stats_avg_period(&chan));

	/* Normal publish */
	zassert_equal(0, zbus_chan_pub(&chan, &val, K_NO_WAIT));
	zassert_equal(1, zbus_chan_pub_stats_count(&chan));
	zassert_within(k_uptime_ticks(), zbus_chan_pub_stats_last_time(&chan), clock_window);
	zassert_within(1000, zbus_chan_pub_stats_avg_period(&chan), 50);

	/* Push 4 times in quick succession, wait for 2 second boundary */
	for (int i = 0; i < 4; i++) {
		zassert_equal(0, zbus_chan_pub(&chan, &val, K_NO_WAIT));
		pub_time = k_uptime_ticks();
	}
	k_sleep(K_TIMEOUT_ABS_MS(2000));
	zassert_equal(5, zbus_chan_pub_stats_count(&chan));
	zassert_within(pub_time, zbus_chan_pub_stats_last_time(&chan), clock_window);
	zassert_within(400, zbus_chan_pub_stats_avg_period(&chan), 50);

	/* Channel claim and finish does not update metadata by default */
	zassert_equal(0, zbus_chan_claim(&chan, K_NO_WAIT));
	zassert_equal(0, zbus_chan_finish(&chan));

	zassert_equal(0, zbus_chan_claim(&chan, K_NO_WAIT));
	cval = zbus_chan_msg(&chan);
	cval->x = 1000;
	zassert_equal(0, zbus_chan_finish(&chan));
	zassert_equal(5, zbus_chan_pub_stats_count(&chan));
	zassert_within(pub_time, zbus_chan_pub_stats_last_time(&chan), clock_window);

	/* Channel notify does not update metadata */
	for (int i = 0; i < 10; i++) {
		zassert_equal(0, zbus_chan_notify(&chan, K_NO_WAIT));
	}
	zassert_equal(5, zbus_chan_pub_stats_count(&chan));
	zassert_within(pub_time, zbus_chan_pub_stats_last_time(&chan), clock_window);

	/* Manually update publish statistics with claim */
	zassert_equal(0, zbus_chan_claim(&chan, K_NO_WAIT));
	zbus_chan_pub_stats_update(&chan);
	pub_time = k_uptime_ticks();
	zassert_equal(0, zbus_chan_finish(&chan));

	k_sleep(K_TIMEOUT_ABS_MS(3000));
	zassert_equal(6, zbus_chan_pub_stats_count(&chan));
	zassert_within(pub_time, zbus_chan_pub_stats_last_time(&chan), clock_window);
	zassert_within(500, zbus_chan_pub_stats_avg_period(&chan), 50);
}

ZTEST_SUITE(publish_stats, NULL, NULL, NULL, NULL, NULL);
