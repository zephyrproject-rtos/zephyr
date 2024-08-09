/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/dt-bindings/clock/ra_clock.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>

static int clock_control_renesas_ra_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_ra_subsys_cfg *subsys_clk = (struct clock_control_ra_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}
	WRITE_BIT(*subsys_clk->mstp, subsys_clk->stop_bit, false);
	return 0;
}

static int clock_control_renesas_ra_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_ra_subsys_cfg *subsys_clk = (struct clock_control_ra_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}

	WRITE_BIT(*subsys_clk->mstp, subsys_clk->stop_bit, true);
	return 0;
}

static int clock_control_renesas_ra_get_rate(const struct device *dev, clock_control_subsys_t sys,
					     uint32_t *rate)
{
	const struct clock_control_ra_pclk_cfg *config = dev->config;
	uint32_t clk_src_rate;
	uint32_t clk_div_val;

	if (!dev || !sys || !rate) {
		return -EINVAL;
	}

	clk_src_rate = R_BSP_SourceClockHzGet(config->clk_src);
	clk_div_val = R_FSP_ClockDividerGet(config->clk_div);
	*rate = clk_src_rate / clk_div_val;
	return 0;
}

/**
 * @brief Initializes a peripheral clock device driver
 */
static int clock_control_ra_init_pclk(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int clock_control_ra_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* Call to HAL layer to initialize system clock and peripheral clock */
	bsp_clock_init();
	return 0;
}

static const struct clock_control_driver_api clock_control_reneas_ra_api = {
	.on = clock_control_renesas_ra_on,
	.off = clock_control_renesas_ra_off,
	.get_rate = clock_control_renesas_ra_get_rate,
};

#define INIT_PCLK(node_id)                                                                         \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, renesas_ra8_cgc_pclk),                              \
		   (static const struct clock_control_ra_pclk_cfg node_id##_cfg =                  \
			    {.clk_src = DT_PROP_OR(node_id, clk_src, RA_CLOCK_SOURCE_DISABLE),     \
			     .clk_div = DT_PROP_OR(node_id, clk_div, RA_SYS_CLOCK_DIV_1)};         \
		    DEVICE_DT_DEFINE(node_id, &clock_control_ra_init_pclk, NULL, NULL,             \
				     &node_id##_cfg, PRE_KERNEL_1,                                 \
				     CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,                          \
				     &clock_control_reneas_ra_api)));

DEVICE_DT_DEFINE(DT_NODELABEL(pclkblock), &clock_control_ra_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, NULL);

DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(pclkblock), INIT_PCLK);
