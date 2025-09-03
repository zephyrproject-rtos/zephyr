/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
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
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_boards.h>
#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_fixed_clock

struct fixed_rate_clock_config {
	uint32_t rate;
	uint32_t system_clock; /* ifx_cat1_clock_block */
};

static int fixed_rate_clk_init(const struct device *dev)
{
	const struct fixed_rate_clock_config *const config = dev->config;

	switch (config->system_clock) {

	case IFX_IMO:
		break;

	case IFX_FLL:
		break;

	case IFX_IHO:
		Cy_SysClk_IhoEnable();
		break;

	default:
		break;
	}

	return 0;
}

#define FIXED_CLK_INIT(n)                                                                          \
	static const struct fixed_rate_clock_config fixed_rate_clock_config_##n = {                \
		.rate = DT_INST_PROP(n, clock_frequency),                                          \
		.system_clock = DT_INST_PROP(n, system_clock),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, fixed_rate_clk_init, NULL, NULL, &fixed_rate_clock_config_##n,    \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
