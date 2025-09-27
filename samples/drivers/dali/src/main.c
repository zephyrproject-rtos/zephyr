/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/dali.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define DALI_NODE DT_NODELABEL(dali0)

int main(void)
{
	LOG_INF("DALI Blinky");
	LOG_INF("Target board: %s", CONFIG_BOARD_TARGET);

	/* get the DALI device */
	const struct device *dali_dev = DEVICE_DT_GET(DALI_NODE);
	if (!device_is_ready(dali_dev)) {
		LOG_ERR("failed to get DALI device.");
		return 0;
	}

	/* prepare on / off frame */
	const struct dali_frame frame_recall_max = (struct dali_frame){
		.event_type = DALI_FRAME_GEAR,
		.data = 0xff05,
	};
	const struct dali_frame frame_off = (struct dali_frame){
		.event_type = DALI_FRAME_GEAR,
		.data = 0xff00,
	};

	/* send on / off DALI frames */
	for (;;) {
		dali_send(dali_dev, &frame_off, NULL, NULL);
		k_sleep(K_MSEC(2000));
		dali_send(dali_dev, &frame_recall_max, NULL, NULL);
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
