/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Clock control driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_def.h>
#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_fixed_factor_clock

struct fixed_factor_clock_config {
	uint32_t divider;
	uint32_t block; /* ifx_cat1_clock_block */
	uint32_t instance;
	uint32_t source_path;
	uint32_t source_instance;
};

static int fixed_factor_clk_init(const struct device *dev)
{
	const struct fixed_factor_clock_config *const config = dev->config;

	switch (config->block) {

	case IFX_CAT1_CLOCK_BLOCK_PATHMUX:
		Cy_SysClk_ClkPathSetSource(config->instance, config->source_path);
		break;

	case IFX_CAT1_CLOCK_BLOCK_HF:
		Cy_SysClk_ClkHfSetSource(config->instance, config->source_instance);
		Cy_SysClk_ClkHfSetDivider(config->instance, config->divider);
		Cy_SysClk_ClkHfEnable(config->instance);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

#define FIXED_CLK_INIT(idx)                                                                        \
	static const struct fixed_factor_clock_config fixed_factor_clock_config_##idx = {          \
		.divider = DT_INST_PROP_OR(idx, clock_divider, 1u),                                \
		.block = DT_INST_PROP(idx, clock_block),                                           \
		.instance = DT_INST_PROP(idx, clock_instance),                                     \
		.source_path = DT_INST_PROP_OR(idx, source_path, 1u),                              \
		.source_instance = DT_INST_PROP_BY_PHANDLE(idx, clocks, clock_instance),           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, fixed_factor_clk_init, NULL, NULL,                              \
			      &fixed_factor_clock_config_##idx, PRE_KERNEL_1,                      \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
