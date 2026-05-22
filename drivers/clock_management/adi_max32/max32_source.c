/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#include <wrap_max32_sys.h>

#define DT_DRV_COMPAT adi_max32_clock_source

struct max32_clock_source_config {
	uint8_t enable_offset;
	uint32_t frequency;
	volatile uint32_t *reg;
	uint8_t enum_val;
};

static int max32_clock_source_configure(const struct clk *clk_hw, const void *data)
{
	const struct max32_clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	if (ungate) {
		MXC_SYS_ClockSourceEnable(config->enum_val);
	} else {
		MXC_SYS_ClockSourceDisable(config->enum_val);
	}

	return 0;
}

static int max32_clock_source_get_rate(const struct clk *clk_hw)
{
	const struct max32_clock_source_config *config = clk_hw->hw_data;

	/* If the clock is enabled, return its frequency, otherwise return 0 */
	return ((*config->reg) & BIT(config->enable_offset)) ? config->frequency : 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t max32_clock_source_configure_recalc(const struct clk *clk_hw, const void *data)
{
	const struct max32_clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	/* If the clock is to be enabled, return its frequency, otherwise return 0 */
	return ungate ? config->frequency : 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t max32_clock_source_best_rate(const struct clk *clk_hw, clock_freq_t rate_req, bool commit)
{
	const struct max32_clock_source_config *config = clk_hw->hw_data;

	/* This clock source can only support its defined frequency, or 0 if gated */
	if (commit) {
		if (rate_req == 0) {
			max32_clock_source_configure(clk_hw, (void *)false);
		} else if (rate_req == config->frequency) {
			max32_clock_source_configure(clk_hw, (void *)true);
		} else {
			return -EINVAL;
		}
	}

	return (rate_req != 0) ? config->frequency : 0;
}
#endif

const struct clock_management_root_api max32_clock_source_api = {
	.shared.configure = max32_clock_source_configure,
	.get_rate = max32_clock_source_get_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.root_configure_recalc = max32_clock_source_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.root_best_rate = max32_clock_source_best_rate,
#endif
};

#define ADI_MAX32_CLOCK_SOURCE_DEFINE(inst) \
	static const struct max32_clock_source_config max32_clock_source_config_##inst = { \
		.frequency = DT_INST_PROP(inst, frequency), \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst), \
		.enable_offset = (uint8_t)DT_INST_PROP(inst, offset), \
		.enum_val = DT_INST_PROP(inst, enum), \
	}; \
	\
	ROOT_CLOCK_DT_INST_DEFINE(inst, \
		&max32_clock_source_config_##inst, \
		&max32_clock_source_api)

DT_INST_FOREACH_STATUS_OKAY(ADI_MAX32_CLOCK_SOURCE_DEFINE)
