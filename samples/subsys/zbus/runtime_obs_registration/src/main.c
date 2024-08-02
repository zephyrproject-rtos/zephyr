/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "messages.h"

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(processed_data_chan, struct sensor_msg, NULL, NULL, ZBUS_OBSERVERS(consumer_sub),
		 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0));

ZBUS_CHAN_DECLARE(raw_data_chan, state_chan);

static void filter_callback(const struct zbus_channel *chan)
{
	const struct sensor_msg *msg = zbus_chan_const_msg(chan);
	struct sensor_msg proc_msg = {0};

	if (0 == (msg->x % 2)) {
		proc_msg.x = msg->x;
	}
	if (0 == (msg->y % 2)) {
		proc_msg.y = msg->y;
	}
	if (0 == (msg->z % 2)) {
		proc_msg.z = msg->z;
	}
	LOG_INF(" -|- Filtering data");
	zbus_chan_pub(&processed_data_chan, &proc_msg, K_MSEC(200));
}

ZBUS_LISTENER_DEFINE(filter_lis, filter_callback);

ZBUS_SUBSCRIBER_DEFINE(state_change_sub, 5);

int main(void)
{
	LOG_INF("System started");

	const struct zbus_channel *chan;

	while (1) {
		LOG_INF("Activating filter");
		zbus_chan_add_obs(&raw_data_chan, &filter_lis, K_MSEC(200));

		zbus_sub_wait(&state_change_sub, &chan, K_FOREVER);

		LOG_INF("Deactivating filter");
		zbus_chan_rm_obs(&raw_data_chan, &filter_lis, K_MSEC(200));

		LOG_INF("Bypass filter");
		zbus_chan_add_obs(&raw_data_chan, &consumer_sub, K_MSEC(200));

		zbus_sub_wait(&state_change_sub, &chan, K_FOREVER);

		LOG_INF("Disable bypass filter");
		zbus_chan_rm_obs(&raw_data_chan, &consumer_sub, K_MSEC(200));

		zbus_sub_wait(&state_change_sub, &chan, K_FOREVER);
	}
	return 0;
}
