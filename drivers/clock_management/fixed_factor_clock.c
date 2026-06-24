/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT fixed_factor_clock

struct fixed_factor_clock_data {
	STANDARD_CLK_SUBSYS_DATA_DEFINE;
	uint32_t divider;
	uint32_t multiplier;
};

static clock_freq_t clock_fixed_factor_recalc_rate(const struct clk *clk_hw,
						   clock_freq_t parent_rate)
{
	const struct fixed_factor_clock_data *config = clk_hw->hw_data;

	if (config->divider == 0 || config->multiplier == 0) {
		return -EINVAL;
	}

	return (parent_rate * config->multiplier) / config->divider;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t clock_fixed_factor_configure_recalc(const struct clk *clk_hw, const void *data,
							clock_freq_t parent_rate)
{
	ARG_UNUSED(data);

	return clock_fixed_factor_recalc_rate(clk_hw, parent_rate);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t clock_fixed_factor_best_rate(const struct clk *clk_hw, clock_freq_t rate_req,
						 clock_freq_t parent_rate, bool commit)
{
	ARG_UNUSED(rate_req);
	ARG_UNUSED(commit);

	return clock_fixed_factor_recalc_rate(clk_hw, parent_rate);
}
#endif

const struct clock_management_standard_api fixed_factor_clock_api = {
	.recalc_rate = clock_fixed_factor_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = clock_fixed_factor_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.best_rate = clock_fixed_factor_best_rate,
#endif
};

#define FIXED_FACTOR_CLOCK_DEFINE(inst)                                                            \
	const struct fixed_factor_clock_data fixed_factor_clock_data_##inst = {                    \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(inst))))     \
			.divider = DT_INST_PROP_OR(inst, clock_div, 1),                            \
		.multiplier = DT_INST_PROP_OR(inst, clock_mult, 1),                                \
	};                                                                                         \
                                                                                                   \
	CLOCK_DT_INST_DEFINE(inst, &fixed_factor_clock_data_##inst, &fixed_factor_clock_api);

DT_INST_FOREACH_STATUS_OKAY(FIXED_FACTOR_CLOCK_DEFINE)
