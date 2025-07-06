/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_cgc_pll

#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>
#include <zephyr/dt-bindings/clock/rx_clock.h>

static int clock_control_renesas_rx_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int clock_control_renesas_rx_pll_off(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static enum clock_control_status clock_control_renesas_rx_pll_get_status(const struct device *dev,
									 clock_control_subsys_t sys)
{
	return CLOCK_CONTROL_STATUS_ON;
}

static int clock_control_renesas_rx_pll_get_rate(const struct device *dev,
						 clock_control_subsys_t sys, uint32_t *rate)
{
	const struct clock_control_rx_pll_cfg *config = dev->config;
	struct clock_control_rx_pll_data *data = dev->data;
	float pll_multiplier;
	float pll_divider;
	uint32_t pll_clock_freq;
	uint32_t clock_dev_freq;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	/* Get the clock frequency of PLL clock device */
	ret = clock_control_get_rate(config->clock_dev, NULL, &clock_dev_freq);
	if (ret) {
		return ret;
	}

	/* Calculate the PLL multiple and divider */
	pll_multiplier = (data->pll_mul + 1) / (2.0);
	pll_divider = data->pll_div;

	/* Calculate PLL clock frequency */
	pll_clock_freq = ((clock_dev_freq / pll_divider) * pll_multiplier);

	*rate = pll_clock_freq;
	return 0;
}

static DEVICE_API(clock_control, clock_control_renesas_rx_pll_api) = {
	.on = clock_control_renesas_rx_pll_on,
	.off = clock_control_renesas_rx_pll_off,
	.get_status = clock_control_renesas_rx_pll_get_status,
	.get_rate = clock_control_renesas_rx_pll_get_rate,
};

#define PLL_CLK_INIT(idx)                                                                          \
	static struct clock_control_rx_pll_cfg pll_cfg##idx = {                                    \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(idx))),                      \
	};                                                                                         \
	static struct clock_control_rx_pll_data pll_data##idx = {                                  \
		.pll_div = DT_INST_PROP(idx, div),                                                 \
		.pll_mul = DT_INST_PROP(idx, mul),                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &pll_data##idx, &pll_cfg##idx, PRE_KERNEL_1,        \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                                  \
			      &clock_control_renesas_rx_pll_api);

DT_INST_FOREACH_STATUS_OKAY(PLL_CLK_INIT);
