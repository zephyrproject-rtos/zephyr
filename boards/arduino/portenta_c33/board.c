/*
 * Copyright 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(eth))
static int eth_clock_enable(void)
{
	int ret;
	const struct device *eth_clk_dev = DEVICE_DT_GET(DT_NODELABEL(eth_clock));

	if (!device_is_ready(eth_clk_dev)) {
		LOG_ERR("Invalid eth_clock device");
		return -ENODEV;
	}

	ret = clock_control_on(eth_clk_dev, (clock_control_subsys_t)0);
	if (ret < 0) {
		LOG_ERR("Failed to enable Ethernet clock, error %d", ret);
		return ret;
	}

	return 0;
}
#endif

void board_late_init_hook(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(eth))
	eth_clock_enable();
#endif
}
