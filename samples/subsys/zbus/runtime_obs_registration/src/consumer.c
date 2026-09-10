/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_SUBSCRIBER_DEFINE(consumer_sub, 4);

static void consumer_subscriber_thread(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&consumer_sub, &chan, K_FOREVER)) {
		struct sensor_msg acc;

		zbus_chan_read(chan, &acc, K_MSEC(500));
		LOG_INF(" --> Consuming data: Acc x=%d, y=%d, z=%d", acc.x, acc.y, acc.z);
	}
}

K_THREAD_DEFINE(consumer_thread_id, 1024, consumer_subscriber_thread, NULL, NULL, NULL, 3, 0, 0);
