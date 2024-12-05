/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_sent_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/sent/sent.h>

#define SENT_NODE DT_INST(0, nxp_s32_sent)

void rx_cb(const struct device *dev, uint8_t channel_id, struct sent_frame *frame,
	   enum sent_state state, void *user_data)
{
	LOG_INF("rx ch %d", channel_id);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(SENT_NODE);

	sent_add_rx_callback(dev, 1, rx_cb, NULL);

	sent_start(dev, 1);

	return 0;
}
