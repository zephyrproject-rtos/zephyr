/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_flexfrg

struct syscon_clock_frg_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	volatile uint32_t *reg;
};

#define SYSCON_FLEXFRGXCTRL_DIV_MASK 0xFF
#define SYSCON_FLEXFRGXCTRL_MULT_MASK 0xFF00

/* Rate calculation helper */
static clock_freq_t syscon_clock_frg_calc_rate(uint64_t parent_rate, uint32_t mult)
{
	/*
	 * Calculate rate. We will use 64 bit integers for this division.
	 * DIV value must be 256, no need to read it
	 */
	return (clock_freq_t)((parent_rate * ((uint64_t)SYSCON_FLEXFRGXCTRL_DIV_MASK + 1ULL)) /
		(mult + SYSCON_FLEXFRGXCTRL_DIV_MASK + 1UL));
}

static clock_freq_t syscon_clock_frg_recalc_rate(const struct clk *clk_hw,
					clock_freq_t parent_rate)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	uint32_t frg_mult;

	frg_mult = FIELD_GET(SYSCON_FLEXFRGXCTRL_MULT_MASK, (*config->reg));
	return syscon_clock_frg_calc_rate(parent_rate, frg_mult);
}

static int syscon_clock_frg_configure(const struct clk *clk_hw, const void *mult)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	uint32_t mult_val = FIELD_PREP(SYSCON_FLEXFRGXCTRL_MULT_MASK, ((uint32_t)mult));

	/* DIV field should always be 0xFF */
	(*config->reg) = mult_val | SYSCON_FLEXFRGXCTRL_DIV_MASK;

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t syscon_clock_frg_configure_recalc(const struct clk *clk_hw,
					     const void *mult,
					     clock_freq_t parent_rate)
{
	uint32_t mult_val = FIELD_PREP(SYSCON_FLEXFRGXCTRL_MULT_MASK, ((uint32_t)mult));

	return syscon_clock_frg_calc_rate(parent_rate, mult_val);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t syscon_clock_frg_round_rate(const struct clk *clk_hw,
				       clock_freq_t rate_req,
				       clock_freq_t parent_rate)
{
	uint32_t mult;

	/* FRG rate is calculated as out_clk = in_clk / (1 + (MULT/DIV)) */
	if (rate_req < parent_rate / 2) {
		/* We can't support this request */
		return -ENOTSUP;
	}
	/*
	 * To calculate a target multiplier value, we use the formula:
	 * MULT = DIV(in_clk - out_clk)/out_clk
	 */
	mult = SYSCON_FLEXFRGXCTRL_DIV_MASK * ((parent_rate - rate_req) /
		rate_req);

	return syscon_clock_frg_calc_rate(parent_rate, mult);
}

static clock_freq_t syscon_clock_frg_set_rate(const struct clk *clk_hw,
					      clock_freq_t rate_req,
					      clock_freq_t parent_rate)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	uint32_t mult, mult_val;
	clock_freq_t output_rate;

	/* FRG rate is calculated as out_clk = in_clk / (1 + (MULT/DIV)) */
	if (rate_req < parent_rate / 2) {
		/* We can't support this request */
		return -ENOTSUP;
	}
	/*
	 * To calculate a target multiplier value, we use the formula:
	 * MULT = DIV(in_clk - out_clk)/out_clk
	 */
	mult = SYSCON_FLEXFRGXCTRL_DIV_MASK * ((parent_rate - rate_req) /
		rate_req);

	mult_val = FIELD_PREP(SYSCON_FLEXFRGXCTRL_MULT_MASK, mult);

	/* Check if multiplier value exceeds mask range- if so, the FRG will
	 * generate a rate equal to input clock divided by 2
	 */
	if (mult > 255) {
		output_rate = parent_rate / 2;
	} else {
		output_rate = syscon_clock_frg_calc_rate(parent_rate, mult);
	}

	/* Apply new configuration */
	(*config->reg) = mult_val | SYSCON_FLEXFRGXCTRL_DIV_MASK;

	return output_rate;
}
#endif

const struct clock_management_standard_api nxp_syscon_frg_api = {
	.recalc_rate = syscon_clock_frg_recalc_rate,
	.shared.configure = syscon_clock_frg_configure,
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	.configure_recalc = syscon_clock_frg_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_frg_round_rate,
	.set_rate = syscon_clock_frg_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_frg_config nxp_syscon_frg_##inst = {         \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_frg_##inst,                           \
			     &nxp_syscon_frg_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
