/*
 * Copyright (c) 2025 by Markus Becker <markushx@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dali.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(D, LOG_LEVEL_INF);

static uint32_t packet_counter;

void dali_rx_callback(const struct device *dev, struct dali_frame frame, void *user_data)
{
	LOG_INF("RX %u %u %x", packet_counter++, frame.event_type, frame.data);
}

#define DALI_NODE DT_NODELABEL(dali0)

int main(void)
{
	LOG_INF("DALI Logger");
	LOG_INF("Target board: %s", CONFIG_BOARD_TARGET);

	/* get the DALI device */
	const struct device *dali_dev = DEVICE_DT_GET(DALI_NODE);

	if (!device_is_ready(dali_dev)) {
		LOG_ERR("failed to get DALI device.");
		return 0;
	}

	dali_receive(dali_dev, dali_rx_callback, NULL);

	/* idle and wait for callbacks */
	for (;;) {
		k_sleep(K_MSEC(60000));
		LOG_INF("ALIVE");
	}
	return 0;
}
