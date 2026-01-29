/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb58_hpsys_pinmux

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

struct pinmux_config {
	uintptr_t base;
	const struct device *clk;
	clock_control_subsys_t subsys;
};

static int pinmux_init(const struct device *dev)
{
	const struct pinmux_config *config = dev->config;
	if (true != device_is_ready(config->clk)) {
		return -ENODEV;
	}

	return clock_control_on(config->clk, config->subsys);
}

static const struct pinmux_config config = {
	.base = DT_REG_ADDR(DT_DRV_INST(0)),
	.clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, id),
};

DEVICE_DT_INST_DEFINE(0, pinmux_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
