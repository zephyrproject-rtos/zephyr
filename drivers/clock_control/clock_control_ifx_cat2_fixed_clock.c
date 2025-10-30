/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat2_fixed_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat2.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_def.h>
#include <cy_sysclk.h>

LOG_MODULE_REGISTER(ifx_cat2_fixed_clock);

struct ifx_cat2_fixed_clock_config {
	uint32_t rate;
	uint32_t id; /* ifx_cat2_clock_block */
};

static int ifx_cat2_fixed_clock_init(const struct device *dev)
{
	int err = CY_SYSCLK_SUCCESS;
	const struct ifx_cat2_fixed_clock_config *const config = dev->config;

	switch (config->id) {
	case IFX_CAT2_CLOCK_BLOCK_IMO:
		if (config->rate > 0) {
			err = Cy_SysClk_ImoSetFrequency(config->rate);
			if (err != CY_SYSCLK_SUCCESS) {
				printk("Failed to set IMO frequency with (error: %d)\n", err);
				return -EIO;
			}

			err = Cy_SysClk_ImoLock(CY_SYSCLK_IMO_LOCK_NONE);
			if (err != CY_SYSCLK_SUCCESS) {
				printk("Failed to set IMO frequency with (error: %d)\n", err);
				return -EIO;
			}
		}
		break;
	case IFX_CAT2_CLOCK_BLOCK_ILO:
		break;
	default:
		break;
	}

	return 0;
}

#define FIXED_CLK_INIT(idx)                                                                        \
	static const struct ifx_cat2_fixed_clock_config ifx_cat2_fixed_clock_config_##idx = {      \
		.rate = DT_INST_PROP(idx, clock_frequency),                                        \
		.id = DT_INST_PROP(idx, clock_block),                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, ifx_cat2_fixed_clock_init, NULL, NULL,                          \
			      &ifx_cat2_fixed_clock_config_##idx, PRE_KERNEL_1,                    \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
