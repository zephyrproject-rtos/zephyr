/*
 * Copyright (c) 2026 Embeint Inc
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
};

ZBUS_CHAN_DEFINE_WITH_ID(chan_a, CHAN_A, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			 ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(chan_b, struct msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

ZTEST(runtime_channel, test_channel_runtime_init_priority)
{
#if defined(CONFIG_ZBUS_PRIORITY_BOOST)
	struct zbus_runtime_channel runtime_chan;
	struct zbus_channel_data data;
	struct msg message;

	zbus_runtime_channel_init(&runtime_chan, &data, "chan1", 0x1234, NULL, &message,
				  sizeof(message), NULL);
	zassert_equal(data.highest_observer_priority, ZBUS_MIN_THREAD_PRIORITY);
#else
	ztest_test_skip();
#endif /* CONFIG_ZBUS_PRIORITY_BOOST */
}

ZTEST(runtime_channel, test_channel_id_collision)
{
	struct zbus_runtime_channel runtime_chan1;
	struct zbus_runtime_channel runtime_chan2;
	struct zbus_channel_data data1;
	struct zbus_channel_data data2;
	struct msg msg1;
	struct msg msg2;

	/* Does not collide with static ZBUS_CHAN_ID_INVALID */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", ZBUS_CHAN_ID_INVALID, NULL,
				  &msg1, sizeof(msg1), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));

	/* Does not collide with other runtime ZBUS_CHAN_ID_INVALID */
	zbus_runtime_channel_init(&runtime_chan2, &data2, "chan2", ZBUS_CHAN_ID_INVALID, NULL,
				  &msg2, sizeof(msg2), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan2));

	zassert_equal(0, zbus_runtime_channel_unregister(&runtime_chan1));
	zassert_equal(0, zbus_runtime_channel_unregister(&runtime_chan2));

	/* Collision with static channel ID */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 100, NULL, &msg1, sizeof(msg1),
				  NULL);
	zassert_equal(-EEXIST, zbus_runtime_channel_register(&runtime_chan1));

	/* Collision with other runtime channel ID */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 101, NULL, &msg1, sizeof(msg1),
				  NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));
	zbus_runtime_channel_init(&runtime_chan2, &data2, "chan2", 101, NULL, &msg2, sizeof(msg2),
				  NULL);
	zassert_equal(-EEXIST, zbus_runtime_channel_register(&runtime_chan2));

	zbus_runtime_channel_unregister_all();
}

ZTEST(runtime_channel, test_channel_retrieval)
{
	struct zbus_runtime_channel runtime_chan1;
	struct zbus_channel_data data1;
	struct msg msg1;

	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 0x12345678, NULL, &msg1,
				  sizeof(msg1), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));

	zassert_equal(&runtime_chan1.channel, zbus_chan_from_id(0x12345678));
	zassert_equal(&runtime_chan1.channel, zbus_chan_from_name("chan1"));

	zassert_equal(0, zbus_runtime_channel_unregister(&runtime_chan1));
	zassert_equal(-ENODATA, zbus_runtime_channel_unregister(&runtime_chan1));

	zbus_runtime_channel_unregister_all();
}

static const struct zbus_channel *channel_match;
static bool channel_found;
static bool terminate_on_match;

static bool iterator_func(const struct zbus_channel *chan)
{
	zassert_not_null(chan);
	if (chan != channel_match) {
		return true;
	}

	channel_found = true;
	return !terminate_on_match;
}

static bool iterator_func_with_ud(const struct zbus_channel *chan, void *user_data)
{
	zassert_equal(user_data, &channel_found);
	return iterator_func(chan);
}

ZTEST(runtime_channel, test_channel_iteration)
{
	struct zbus_runtime_channel runtime_chan1;
	struct zbus_channel_data data1;
	struct msg msg1;
	bool all_executed;

	channel_match = &runtime_chan1.channel;

	/* No matches before registration */
	all_executed = zbus_iterate_over_channels(iterator_func);
	zassert_true(all_executed);
	zassert_false(channel_found);
	all_executed =
		zbus_iterate_over_channels_with_user_data(iterator_func_with_ud, &channel_found);
	zassert_true(all_executed);
	zassert_false(channel_found);

	/* Register the channel */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 0x12345678, NULL, &msg1,
				  sizeof(msg1), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));

	/* Match found, but iteration continues */
	all_executed = zbus_iterate_over_channels(iterator_func);
	zassert_true(all_executed);
	zassert_true(channel_found);
	channel_found = false;
	all_executed =
		zbus_iterate_over_channels_with_user_data(iterator_func_with_ud, &channel_found);
	zassert_true(all_executed);
	zassert_true(channel_found);

	/* Match found, iteration terminates */
	terminate_on_match = true;
	all_executed = zbus_iterate_over_channels(iterator_func);
	zassert_false(all_executed);
	zassert_true(channel_found);
	channel_found = false;
	all_executed =
		zbus_iterate_over_channels_with_user_data(iterator_func_with_ud, &channel_found);
	zassert_false(all_executed);
	zassert_true(channel_found);
}

static bool publish_validator(const void *msg, size_t msg_size)
{
	const struct msg *m = msg;

	/* Simple validator */
	return m->x < 1000;
}

ZTEST(runtime_channel, test_channel_publish_flow)
{
	const struct zbus_channel *chan;
	struct zbus_runtime_channel runtime_chan1;
	struct zbus_channel_data data1;
	struct msg msg1;
	struct msg read;
	struct msg pub;
	int rc;

	/* Register runtime channel */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 0x1234, publish_validator, &msg1,
				  sizeof(msg1), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));

	/* After registration, the runtime channel behaves exactly the same as a standard channel */
	chan = zbus_chan_from_id(0x1234);

	/* Publish data on runtime channel */
	pub.x = 10;
	rc = zbus_chan_pub(chan, &pub, K_FOREVER);
	zassert_equal(0, rc);

	/* Internal storage updated */
	zassert_mem_equal(&msg1, &pub, sizeof(pub));

	/* Read data from runtime channel */
	rc = zbus_chan_read(chan, &read, K_FOREVER);
	zassert_equal(0, rc);
	zassert_equal(pub.x, read.x);

	/* Publish invalid data */
	pub.x = 1001;
	rc = zbus_chan_pub(chan, &pub, K_FOREVER);
	zassert_equal(-ENOMSG, rc);

	/* Data is still the old valid data */
	rc = zbus_chan_read(chan, &read, K_FOREVER);
	zassert_equal(0, rc);
	zassert_equal(10, read.x);

	zbus_runtime_channel_unregister_all();
}

static int count_callback1;
static void callback1(const struct zbus_channel *chan)
{
	++count_callback1;
}

ZBUS_LISTENER_DEFINE(lis1, callback1);

ZTEST(runtime_channel, test_channel_observers)
{
	const struct zbus_channel *chan;
	struct zbus_runtime_channel runtime_chan1;
	struct zbus_channel_data data1;
	struct msg msg1;
	struct msg pub;
	int rc;

	/* Register runtime channel and an observer */
	zbus_runtime_channel_init(&runtime_chan1, &data1, "chan1", 0x1234, publish_validator, &msg1,
				  sizeof(msg1), NULL);
	zassert_equal(0, zbus_runtime_channel_register(&runtime_chan1));
	chan = zbus_chan_from_id(0x1234);
	zassert_equal(0, zbus_chan_add_obs(chan, &lis1, K_MSEC(200)));

	/* Publish data on runtime channel */
	pub.x = 10;
	rc = zbus_chan_pub(chan, &pub, K_FOREVER);
	zassert_equal(0, rc);

	/* Observer was notified */
	zassert_equal(count_callback1, 1);

	/* Cleanup */
	zassert_equal(0, zbus_chan_rm_obs(chan, &lis1, K_MSEC(200)));
	zbus_runtime_channel_unregister_all();
}

void test_after(void *fixture)
{
	zbus_runtime_channel_unregister_all();
}

ZTEST_SUITE(runtime_channel, NULL, NULL, NULL, test_after, NULL);
