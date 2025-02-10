/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sent_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/sent/sent.h>

#define SENT_NODE    DT_ALIAS(sent_node)
#define SENT_CHANNEL 1

void rx_cb(const struct device *dev, uint8_t channel_id, struct sent_frame *frame,
	   enum sent_status status, void *user_data)
{
	LOG_INF("Rx channel %d completed\n", channel_id);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(SENT_NODE);

	sent_add_rx_callback(dev, SENT_CHANNEL, rx_cb, NULL);

	sent_start_rx(dev, SENT_CHANNEL);

	k_sleep(K_MSEC(100));

	sent_stop_rx(dev, SENT_CHANNEL);

	return 0;
}
