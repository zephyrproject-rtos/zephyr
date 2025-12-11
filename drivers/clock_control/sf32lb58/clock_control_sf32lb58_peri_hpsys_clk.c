/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb58_peri_hpsys_clk

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/drivers/clock_control/sf32lb58_clock_control.h>
#include <zephyr/dt-bindings/clock/sf32lb58_clock.h>

struct peri_hpsys_clk_config {
	uint32_t clk_freq;
	const struct device *clk_dev;
};

static int peri_hpsys_clk_init(const struct device *dev)
{
	return 0;
}

static int peri_hpsys_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct peri_hpsys_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->aon_dev), "aon_dev is not ready");
	return clock_control_on(config->clk_dev, (clock_control_subsys_t)0);
}

static int peri_hpsys_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct peri_hpsys_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->aon_dev), "aon_dev is not ready");
	return clock_control_off(config->clk_dev, (clock_control_subsys_t)0);
}

static enum clock_control_status peri_hpsys_clk_get_status(const struct device *dev,
							   clock_control_subsys_t sys)
{
	const struct peri_hpsys_clk_config *config = dev->config;
	ARG_UNUSED(sys);
	__ASSERT(true == device_is_ready(config->clk_dev), "clk_dev is not ready");
	return clock_control_get_status(config->clk_dev, (clock_control_subsys_t)0);
}

static int peri_hpsys_clk_get_rate(const struct device *dev, clock_control_subsys_t subsys,
				   uint32_t *rate)
{
	ARG_UNUSED(subsys);
	*rate = ((const struct peri_hpsys_clk_config *)dev->config)->clk_freq;
	return 0;
}

static DEVICE_API(clock_control, peri_hpsys_clk_api) = {
	.on = peri_hpsys_clk_on,
	.off = peri_hpsys_clk_off,
	.get_status = peri_hpsys_clk_get_status,
	.get_rate = peri_hpsys_clk_get_rate,
};

static struct peri_hpsys_clk_config config = {
	.clk_freq = DT_INST_PROP(0, clock_frequency),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
};

DEVICE_DT_INST_DEFINE(0, peri_hpsys_clk_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &peri_hpsys_clk_api);
