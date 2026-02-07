/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb58_xt_clk

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/drivers/clock_control/sf32lb58_clock_control.h>
#include <zephyr/dt-bindings/clock/sf32lb58_clock.h>

struct xt_clk_config {
	uint32_t clk_freq;
	const struct device *aon_dev;
	clock_control_subsys_t aon_subsys;
};

static int xt_clk_init(const struct device *dev)
{
	return 0;
}

static int xt_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xt_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->aon_dev), "aon_dev is not ready");
	return clock_control_on(config->aon_dev, config->aon_subsys);
}

static int xt_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xt_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->aon_dev), "aon_dev is not ready");
	return clock_control_off(config->aon_dev, config->aon_subsys);
}

static enum clock_control_status xt_clk_get_status(const struct device *dev,
						   clock_control_subsys_t sys)
{
	const struct xt_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->aon_dev), "aon_dev is not ready");
	return clock_control_get_status(config->aon_dev, config->aon_subsys);
}

static int xt_clk_get_rate(const struct device *dev, clock_control_subsys_t subsys, uint32_t *rate)
{
	ARG_UNUSED(subsys);
	*rate = ((const struct xt_clk_config *)dev->config)->clk_freq;
	return 0;
}

static DEVICE_API(clock_control, xt_clk_api) = {
	.on = xt_clk_on,
	.off = xt_clk_off,
	.get_status = xt_clk_get_status,
	.get_rate = xt_clk_get_rate,
};

static struct xt_clk_config config = {
	.clk_freq = DT_INST_PROP(0, clock_frequency),
	.aon_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.aon_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, subsys),
};

DEVICE_DT_INST_DEFINE(0, xt_clk_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &xt_clk_api);
