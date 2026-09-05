/*
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT beken_bk7258_clock_controller

#include <errno.h>

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/beken_bk7258_clock.h>

static const uint32_t bk7258_clk_bits[] = {
	[BK7258_CLK_UART1] = BIT(10),
};

struct clock_control_bk7258_config {
	mem_addr_t reg_base;
};

static int bk7258_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_bk7258_config *cfg = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;

	if (id >= ARRAY_SIZE(bk7258_clk_bits)) {
		return -EINVAL;
	}

	sys_set_bits(cfg->reg_base, bk7258_clk_bits[id]);

	return 0;
}

static int bk7258_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_bk7258_config *cfg = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;

	if (id >= ARRAY_SIZE(bk7258_clk_bits)) {
		return -EINVAL;
	}

	sys_clear_bits(cfg->reg_base, bk7258_clk_bits[id]);

	return 0;
}

static DEVICE_API(clock_control, bk7258_clock_api) = {
	.on = bk7258_clock_on,
	.off = bk7258_clock_off,
};

#define BK7258_CLOCK_INIT(inst)                                                                    \
	static const struct clock_control_bk7258_config clock_control_bk7258_config_##inst = {     \
		.reg_base = DT_INST_REG_ADDR(inst),                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &clock_control_bk7258_config_##inst,         \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &bk7258_clock_api);

DT_INST_FOREACH_STATUS_OKAY(BK7258_CLOCK_INIT)
