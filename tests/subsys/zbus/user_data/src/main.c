/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "messages.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 2) /* Initial value major 0, minor 1, build 1023 */
);

static int my_user_data;

ZBUS_CHAN_DEFINE(regular_chan,	 /* Name */
		 struct foo_msg, /* Message type */

		 NULL,					       /* Validator */
		 &my_user_data,				       /* User data */
		 ZBUS_OBSERVERS(foo_listener, foo_subscriber), /* observers */
		 ZBUS_MSG_INIT(0) /* Initial value major 0, minor 1, build 1023 */
);

ZTEST(user_data, test_channel_user_data)
{
	zassert_true(sizeof(my_user_data) > 0, NULL);

	zassert_equal_ptr(version_chan.user_data, NULL, NULL);

	zassert_equal_ptr(regular_chan.user_data, &my_user_data, NULL);

	int *counter = regular_chan.user_data;
	*counter = -2;

	zassert_equal(zbus_chan_user_data(&regular_chan), counter, NULL);
	zassert_equal(*(int *)zbus_chan_user_data(&regular_chan), -2, NULL);

	memset(regular_chan.user_data, 0, sizeof(my_user_data));
}

static void urgent_callback(const struct zbus_channel *chan)
{
	if (chan == &(regular_chan)) {
		int *count = zbus_chan_user_data(&regular_chan);

		*count += 1;
	}
}

ZBUS_LISTENER_DEFINE(foo_listener, urgent_callback);

ZBUS_SUBSCRIBER_DEFINE(foo_subscriber, 1);

static void foo_subscriber_thread(void)
{
	struct zbus_channel *chan = NULL;

	while (1) {
		if (!k_msgq_get(foo_subscriber.queue, &chan, K_FOREVER)) {
			if (chan == &(regular_chan)) {
				if (!zbus_chan_claim(&regular_chan, K_FOREVER)) {
					int *count = zbus_chan_user_data(&regular_chan);

					*count += 1;

					zbus_chan_finish(&regular_chan);
				}
			}
		}
	}
}

K_THREAD_DEFINE(foo_subscriber_thread_id, 1024, foo_subscriber_thread, NULL, NULL, NULL, 3, 0, 0);

ZTEST(user_data, test_user_data_regression)
{
	/* To ensure the pub/sub behavior is kept */
	struct foo_msg sent = {.a = 10, .b = 1000};

	zbus_chan_pub(&regular_chan, &sent, K_MSEC(100));

	struct foo_msg received;

	zbus_chan_read(&regular_chan, &received, K_MSEC(100));

	zassert_equal(sent.a, received.a, NULL);

	zassert_equal(sent.b, received.b, NULL);

	k_msleep(1000);

	if (!zbus_chan_claim(&regular_chan, K_FOREVER)) {
		int *count = zbus_chan_user_data(&regular_chan);

		zassert_equal(*count, 2, NULL);

		zbus_chan_finish(&regular_chan);
	}
}

ZTEST_SUITE(user_data, NULL, NULL, NULL, NULL, NULL);
