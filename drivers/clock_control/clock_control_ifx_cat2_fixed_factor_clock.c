/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat2_fixed_factor_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat2.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_def.h>
#include <cy_sysclk.h>
#include <cy_syslib.h>

LOG_MODULE_REGISTER(ifx_cat2_fixed_factor_clock);

struct ifx_cat2_fixed_factor_config {
	uint32_t divider;
	uint32_t block; /* ifx_cat2_clock_block */
	uint32_t instance;
	uint32_t source_instance;
};

static int ifx_cat2_fixed_factor_init(const struct device *dev)
{
	int err = CY_SYSCLK_SUCCESS;
	const struct ifx_cat2_fixed_factor_config *const config = dev->config;

	switch (config->block) {
	case IFX_CAT2_BLOCK_HFCLK_SEL:
		break;
	case IFX_CAT2_BLOCK_PUMP_SEL:
		break;
	case IFX_CAT2_BLOCK_WDT_CLKEN:
		break;
	case IFX_CAT2_CLOCK_BLOCK_HF:
		/* Set worst case memory wait states (48 MHz), will update at the end */
		Cy_SysLib_SetWaitStates(48);

		/* Map source identifiers to Infineon HAL identifiers */
		cy_en_sysclk_clkhf_src_t hal_source = CY_SYSCLK_CLKHF_IN_IMO;

		switch (config->source_instance) {
		case IFX_CAT2_CLKPATH_IN_IMO:
			hal_source = CY_SYSCLK_CLKHF_IN_IMO;
			break;
		case IFX_CAT2_CLKPATH_IN_EXTCLK:
			hal_source = CY_SYSCLK_CLKHF_IN_EXTCLK;
			break;
		default:
			hal_source = CY_SYSCLK_CLKHF_IN_IMO;
		}

		err = Cy_SysClk_ClkHfSetSource(hal_source);
		if (err != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to set clock high frequency source %d\n", err);
			return -EIO;
		}

		/* Map your divider to Infineon HAL divider */
		cy_en_sysclk_dividers_t hal_divider = CY_SYSCLK_NO_DIV;

		switch (config->divider) {
		case IFX_CAT2_CLKHF_NO_DIVIDE:
			hal_divider = CY_SYSCLK_NO_DIV;
			break;
		case IFX_CAT2_CLKHF_DIV_BY_2:
			hal_divider = CY_SYSCLK_DIV_2;
			break;
		case IFX_CAT2_CLKHF_DIV_BY_4:
			hal_divider = CY_SYSCLK_DIV_4;
			break;
		case IFX_CAT2_CLKHF_DIV_BY_8:
			hal_divider = CY_SYSCLK_DIV_8;
			break;
		default:
			hal_divider = CY_SYSCLK_NO_DIV;
		}

		/* Configure HFCLK - set divider first, then source */
		Cy_SysClk_ClkHfSetDivider(hal_divider);
		Cy_SysClk_ClkSysSetDivider(hal_divider);

		SystemCoreClockUpdate();

		err = Cy_SysClk_ClkPumpSetSource(CY_SYSCLK_PUMP_IN_GND);
		if (err != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to set clock pump set source %d\n", err);
			return -EIO;
		}

		/* Set wait states BEFORE if frequency is increasing */
		Cy_SysLib_SetWaitStates(Cy_SysClk_ClkSysGetFrequency() / 1000000UL);

		/* Update System Core Clock values for correct Cy_SysLib_Delay functioning */
		SystemCoreClockUpdate();
		break;

	default:
		return -EIO;
	}

	return 0;
}

#define FIXED_CLK_INIT(idx)                                                                        \
	static const struct ifx_cat2_fixed_factor_config ifx_cat2_fixed_factor_config_##idx = {    \
		.divider = DT_INST_PROP_OR(idx, clock_divider, IFX_CAT2_CLKHF_NO_DIVIDE),          \
		.block = DT_INST_PROP(idx, clock_block),                                           \
		.instance = DT_INST_PROP(idx, clock_instance),                                     \
		.source_instance = DT_INST_PROP_BY_PHANDLE(idx, clocks, clock_instance),           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, ifx_cat2_fixed_factor_init, NULL, NULL,                         \
			      &ifx_cat2_fixed_factor_config_##idx, PRE_KERNEL_1,                   \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
