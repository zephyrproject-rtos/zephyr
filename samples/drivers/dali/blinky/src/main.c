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
	const struct device *dali_dev = DEVICE_DT_GET(DALI_NODE);
	int ret;

	LOG_INF("DALI Blinky");
	LOG_INF("Target board: %s", CONFIG_BOARD_TARGET);

	if (!device_is_ready(dali_dev)) {
		LOG_ERR("failed to get DALI device.");
		return 0;
	}

	/* DALI frame that commands max brightness */
	/* see iec 62386-102 for full details */
	/* data low byte - opcode byte : 0x05 - RECALL MAX LEVEL */
	/* data high byte - address byte : 0xFF - broadcast */
	const struct dali_frame frame_recall_max = (struct dali_frame){
		.event_type = DALI_FRAME_GEAR,
		.data = 0xff05,
	};
	/* DALI frame that commands off */
	/* see iec 62386-102 for full details */
	/* data low byte - opcode byte : 0x00 - OFF */
	/* data high byte - address byte : 0xFF - broadcast */
	const struct dali_frame frame_off = (struct dali_frame){
		.event_type = DALI_FRAME_GEAR,
		.data = 0xff00,
	};

	/* transmit on / off DALI frames */
	for (;;) {
		ret = dali_transmit(dali_dev, &frame_recall_max, NULL, NULL);
		if (ret < 0) {
			LOG_ERR("failed to transmit recall max with error code %d", ret);
			return -1;
		}
		k_sleep(K_MSEC(2000));
		ret = dali_transmit(dali_dev, &frame_off, NULL, NULL);
		if (ret < 0) {
			LOG_ERR("failed to transmit off with error code %d", ret);
			return -1;
		}
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
