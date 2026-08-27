/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

void peripheral_thread(void)
{
	struct sensor_msg sm = {0};

	while (1) {
		LOG_DBG("Sending sensor data...");

		sm.press += 1;
		sm.temp += 10;
		sm.humidity += 100;

		zbus_chan_pub(&sensor_data_chan, &sm, K_MSEC(250));

		k_msleep(500);
	}
}

K_THREAD_DEFINE(peripheral_thread_id, 1024, peripheral_thread, NULL, NULL, NULL, 5, 0, 0);
