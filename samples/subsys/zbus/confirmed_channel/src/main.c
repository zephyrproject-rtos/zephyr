/*
 * Copyright (c) 2023 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

static atomic_t sub_count = ATOMIC_INIT(0);

struct confirmed_msg {
	uint32_t payload;
};

ZBUS_CHAN_DEFINE(confirmed_chan,       /* Name */
		 struct confirmed_msg, /* Message type */

		 NULL,                                                  /* Validator */
		 &sub_count,                                            /* User data */
		 ZBUS_OBSERVERS(foo_lis, bar_sub1, bar_sub2, bar_sub3), /* observers */
		 ZBUS_MSG_INIT(.payload = 0)                            /* Initial value */
);

static void listener_callback_example(const struct zbus_channel *chan)
{
	const struct confirmed_msg *cm = zbus_chan_const_msg(chan);

	LOG_INF("From listener -> Confirmed message payload = %u", cm->payload);
}

ZBUS_LISTENER_DEFINE(foo_lis, listener_callback_example);

ZBUS_SUBSCRIBER_DEFINE(bar_sub1, 4);

static void bar_sub1_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&bar_sub1, &chan, K_FOREVER)) {
		struct confirmed_msg cm;

		if (&confirmed_chan != chan) {
			continue;
		}

		zbus_chan_read(&confirmed_chan, &cm, K_MSEC(500));

		k_msleep(2500);

		atomic_dec(zbus_chan_user_data(&confirmed_chan));

		LOG_INF("From bar_sub1 subscriber -> Confirmed "
			"message payload = "
			"%u",
			cm.payload);
	}
}
K_THREAD_DEFINE(bar_sub1_task_id, CONFIG_MAIN_STACK_SIZE, bar_sub1_task, NULL, NULL, NULL, 3, 0, 0);
ZBUS_SUBSCRIBER_DEFINE(bar_sub2, 4);
static void bar_sub2_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&bar_sub2, &chan, K_FOREVER)) {
		struct confirmed_msg cm;

		if (&confirmed_chan != chan) {
			continue;
		}

		zbus_chan_read(&confirmed_chan, &cm, K_MSEC(500));

		k_msleep(1000);

		atomic_dec(zbus_chan_user_data(&confirmed_chan));

		LOG_INF("From bar_sub2 subscriber -> Confirmed "
			"message payload = "
			"%u",
			cm.payload);
	}
}
K_THREAD_DEFINE(bar_sub2_task_id, CONFIG_MAIN_STACK_SIZE, bar_sub2_task, NULL, NULL, NULL, 3, 0, 0);

ZBUS_SUBSCRIBER_DEFINE(bar_sub3, 4);
static void bar_sub3_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&bar_sub3, &chan, K_FOREVER)) {
		struct confirmed_msg cm;

		if (&confirmed_chan != chan) {
			continue;
		}

		zbus_chan_read(&confirmed_chan, &cm, K_MSEC(500));

		k_msleep(5000);

		atomic_dec(zbus_chan_user_data(&confirmed_chan));

		LOG_INF("From bar_sub3 subscriber -> Confirmed "
			"message payload = "
			"%u",
			cm.payload);
	}
}
K_THREAD_DEFINE(bar_sub3_task_id, CONFIG_MAIN_STACK_SIZE, bar_sub3_task, NULL, NULL, NULL, 3, 0, 0);

static void pub_to_confirmed_channel(struct confirmed_msg *cm)
{
	/* Wait for channel be consumed */
	while (atomic_get(zbus_chan_user_data(&confirmed_chan)) > 0) {
		k_msleep(100);
	}
	/* Set the number of subscribers to consume the channel */
	atomic_set(zbus_chan_user_data(&confirmed_chan), 3);

	zbus_chan_pub(&confirmed_chan, cm, K_MSEC(500));
}

int main(void)
{
	struct confirmed_msg cm = {0};

	while (1) {
		pub_to_confirmed_channel(&cm);

		++cm.payload;
	}

	return 0;
}
