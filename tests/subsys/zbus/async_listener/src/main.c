/*
 * Copyright (c) 2025 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct msg_bool {
	bool is_valid;
};

ZBUS_CHAN_DEFINE(chan_int,        /* Name */
		 struct msg_bool, /* Message type */

		 NULL,                            /* Validator */
		 NULL,                            /* User data */
		 ZBUS_OBSERVERS(alis1, alis2),    /* observers */
		 ZBUS_MSG_INIT(.is_valid = false) /* Initial value */
);

K_SEM_DEFINE(sem_alis_test, 0, CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE);

static void async_listener_callback(const struct zbus_channel *chan, const void *message)
{
	if (chan != &chan_int) {
		return;
	}

	const struct msg_bool *msg = message;

	if (msg->is_valid) {
		k_sem_give(&sem_alis_test);
	}
}

ZBUS_ASYNC_LISTENER_DEFINE(alis1, async_listener_callback);

ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE(alis2, NULL, false);

static void isr_handler(const void *message)
{
	const struct msg_bool *msg = message;

	zbus_chan_pub(&chan_int, msg, K_NO_WAIT);
}

static void isr_burst_handler(const void *ptr)
{
	ARG_UNUSED(ptr);

	struct msg_bool msg = {.is_valid = true};

	int expected_failing_i = CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE - 1;

	for (int i = 0; i < CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE; ++i) {
		int err = zbus_chan_pub(&chan_int, &msg, K_NO_WAIT);
		if (err) {
			zassert_equal(err, -ENOMEM);
			zassert_equal(i, expected_failing_i);
			return;
		}
	}
}

ZTEST(async_listener, test_specification)
{
	struct msg_bool msg = {.is_valid = true};

	zbus_chan_pub(&chan_int, &msg, K_MSEC(200));
	zassert_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));

	msg.is_valid = false;
	zbus_chan_pub(&chan_int, &msg, K_MSEC(200));

	/* This must timeout the sem take because the x is not greater than zero */
	zassert_not_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));

	/* Publications from ISR must have the same behavior as the previous tests */
	msg.is_valid = true;
	irq_offload(isr_handler, &msg);
	zassert_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));

	msg.is_valid = false;
	irq_offload(isr_handler, &msg);
	zassert_not_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));

	/* Enabling an invalid async listener */

	msg.is_valid = true;
	irq_offload(isr_burst_handler, NULL);
	/* We can only publish up to CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE - 1 */
	for (int i = CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE - 1; i > 0; --i) {
		zassert_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));
	}
	zassert_not_ok(k_sem_take(&sem_alis_test, K_MSEC(100)));
}

ZTEST_SUITE(async_listener, NULL, NULL, NULL, NULL, NULL);
