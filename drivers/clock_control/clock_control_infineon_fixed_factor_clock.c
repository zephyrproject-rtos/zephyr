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
#include <zephyr/logging/log.h>
#include <stdlib.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_boards.h>

#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_fixed_factor_clock

LOG_MODULE_REGISTER(clock_control_ifx_fixed_factor_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct fixed_factor_clock_config {
	uint32_t divider;
	uint32_t block; /* ifx_cat1_clock_block */
	uint32_t instance;
	uint32_t source_path;
};

#if defined(CONFIG_SOC_SERIES_PSE84) || defined(CONFIG_SOC_SERIES_PSC3)
static int check_legal_max_min(const struct device *dev)
{
	const struct fixed_factor_clock_config *const config = dev->config;

#if defined(CONFIG_SOC_SERIES_PSE84)
	if (config->block == IFX_HF && config->instance == 0) {
		if (Cy_SysClk_ClkHfGetFrequency(0) > MHZ(200)) {
			LOG_ERR("clk_hf0 frequency is greater than legal max 200 MHz");
			return -EINVAL;
		}
	}
#elif defined(CONFIG_SOC_SERIES_PSC3)
	if (config->block == IFX_HF && config->instance == 0) {
		if (Cy_SysClk_ClkHfGetFrequency(0) > MHZ(180)) {
			LOG_ERR("clk_hf0 frequency is greater than legal max 180 MHz");
			return -EINVAL;
		}
	}
#endif

	return 0;
}
#endif

static int fixed_factor_clk_init(const struct device *dev)
{
	const struct fixed_factor_clock_config *const config = dev->config;
#if defined(CONFIG_SOC_SERIES_PSE84) || defined(CONFIG_SOC_SERIES_PSC3)
	uint32_t rslt;
#endif
	uint32_t err;

	switch (config->block) {

	case IFX_PATHMUX:
#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		Cy_SysClk_ClkPathSetSource(config->instance, config->source_path);
#endif
		break;

	case IFX_HF:
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		err = Cy_SysClk_ClkHfSetSource(config->source_path);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}

		Cy_SysClk_ClkHfSetDivider(config->divider);
#else
		err = Cy_SysClk_ClkHfSetSource(config->instance, source_instance);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}

		err = Cy_SysClk_ClkHfSetDivider(config->instance, config->divider);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}

		err = Cy_SysClk_ClkHfEnable(config->instance);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}
#endif
		break;

	case IFX_PUMP:
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		err = Cy_SysClk_ClkPumpSetSource(config->source_path);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}
#endif
		break;

	default:
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_PSE84) || defined(CONFIG_SOC_SERIES_PSC3)
	rslt = check_legal_max_min(dev);
#endif
	return 0;
}

#define FIXED_CLK_INIT(n)                                                                          \
	static const struct fixed_factor_clock_config fixed_factor_clock_config_##n = {            \
		.divider = DT_INST_PROP_OR(n, clock_div, 1u),                                      \
		.block = DT_INST_PROP(n, system_clock),                                            \
		.instance = DT_INST_PROP(n, instance),                                             \
		.source_path = DT_INST_PROP_OR(n, source_path, 1u),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, fixed_factor_clk_init, NULL, NULL,                                \
			      &fixed_factor_clock_config_##n, PRE_KERNEL_1,                        \
			      CONFIG_CLOCK_CONTROL_IFX_FIXED_FACTOR_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
