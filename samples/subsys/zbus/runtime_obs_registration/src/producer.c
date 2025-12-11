/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(raw_data_chan, struct sensor_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0));

ZBUS_CHAN_DEFINE(state_chan, bool, NULL, NULL, ZBUS_OBSERVERS(state_change_sub), false);

static void producer_thread(void)
{
	struct sensor_msg acc = {0};
	uint32_t count = 0;

	while (1) {
		if (count == 5) {
			bool change_state = true;

			zbus_chan_pub(&state_chan, &change_state, K_MSEC(200));
			count = 0;
		}

		++acc.x;
		++acc.y;
		++acc.z;

		LOG_INF(" >-- Raw data fetched");

		zbus_chan_pub(&raw_data_chan, &acc, K_MSEC(200));

		k_msleep(500);

		++count;
	}
}

K_THREAD_DEFINE(produce_thread_id, 1024, producer_thread, NULL, NULL, NULL, 3, 0, 0);
