/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DECLARE(start_measurement_chan);

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 1023) /* Initial value major 0, minor 1, build 1023 */
);

static void core_thread(void)
{
	struct action_msg start = {false};

	while (1) {
		LOG_DBG("Core sending start measurement with status %d", start.status);

		start.status = !start.status;
		zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(500));

		k_msleep(1000);
	}
}

K_THREAD_DEFINE(core_thread_id, 1024, core_thread, NULL, NULL, NULL, 3, 0, 0);
