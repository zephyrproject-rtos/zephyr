/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_cgc_root_clock

#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>

static int clock_control_renesas_rx_root_on(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int clock_control_renesas_rx_root_off(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int clock_control_renesas_rx_root_get_rate(const struct device *dev,
						  clock_control_subsys_t sys, uint32_t *rate)
{
	const struct clock_control_rx_root_cfg *config = dev->config;

	ARG_UNUSED(sys);

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	*rate = config->rate;
	return 0;
}

static int clock_control_rx_init(const struct device *dev)
{
	ARG_UNUSED(dev);
#if CONFIG_HAS_RENESAS_RX_RDP
	/* Call to HAL layer to initialize system clock and peripheral clock */
	mcu_clock_setup();
#endif
	return 0;
}

static DEVICE_API(clock_control, clock_control_renesas_rx_root_api) = {
	.on = clock_control_renesas_rx_root_on,
	.off = clock_control_renesas_rx_root_off,
	.get_rate = clock_control_renesas_rx_root_get_rate,
};

#define ROOT_CLK_INIT(idx)                                                                         \
	static const struct clock_control_rx_root_cfg clock_control_rx_root_cfg##idx = {           \
		.rate = DT_INST_PROP(idx, clock_frequency),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, &clock_control_rx_init, NULL, NULL,                             \
			      &clock_control_rx_root_cfg##idx, PRE_KERNEL_1,                       \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                                  \
			      &clock_control_renesas_rx_root_api);

DT_INST_FOREACH_STATUS_OKAY(ROOT_CLK_INIT);
