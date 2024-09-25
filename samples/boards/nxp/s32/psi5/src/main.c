/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_psi5_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE DT_INST(0, nxp_s32_psi5)

void tx_cb(const struct device *dev, uint8_t channel_id, enum psi5_state state, void *user_data)
{
	LOG_INF("tx ch %d", channel_id);
}

void rx_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
	   enum psi5_state state, void *user_data)
{
	LOG_INF("rx ch %d", channel_id);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(PSI5_NODE);
	uint64_t send_data = 0x123456789ABCDEF;

	psi5_add_rx_callback(dev, 2, rx_cb, NULL);

	psi5_start(dev, 1);

	psi5_send(dev, 1, send_data, K_MSEC(100), tx_cb, NULL);

	psi5_stop(dev, 1);

	k_sleep(K_MSEC(100));

	return 0;
}
