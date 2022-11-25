/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

static bool sensor_from_bridge;
ZBUS_CHAN_DEFINE(sensor_data_chan,	 /* Name */
		 struct sensor_data_msg, /* Message type */

		 NULL,			    /* Validator */
		 &sensor_from_bridge,			    /* Validator */
		 ZBUS_OBSERVERS(proxy_lis), /* observers */
		 ZBUS_MSG_INIT(0)	    /* Initial value {0} */
);

ZBUS_SUBSCRIBER_DEFINE(peripheral_sub, 8);

static void peripheral_thread(void)
{
	const struct zbus_channel *chan;
	struct sensor_data_msg sd = {0};

	while (!zbus_sub_wait(&peripheral_sub, &chan, K_FOREVER)) {
		sd.value += 1;

		LOG_DBG("sending sensor data err = %d",
			zbus_chan_pub(&sensor_data_chan, &sd, K_MSEC(250)));
	}
}

K_THREAD_DEFINE(peripheral_thread_id, 1024, peripheral_thread, NULL, NULL, NULL, 5, 0, 0);
