/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_flexfrg

struct syscon_clock_frg_config {
	const struct clk *parent;
	volatile uint32_t *reg;
};

#define SYSCON_FLEXFRGXCTRL_DIV_MASK 0xFF
#define SYSCON_FLEXFRGXCTRL_MULT_MASK 0xFF00

/* Rate calculation helper */
static uint32_t syscon_clock_frg_calc_rate(uint64_t parent_rate, uint32_t mult)
{
	/*
	 * Calculate rate. We will use 64 bit integers for this division.
	 * DIV value must be 256, no need to read it
	 */
	return (uint32_t)((parent_rate * ((uint64_t)SYSCON_FLEXFRGXCTRL_DIV_MASK + 1ULL)) /
		(mult + SYSCON_FLEXFRGXCTRL_DIV_MASK + 1UL));
}

static int syscon_clock_frg_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	uint32_t frg_mult;

	if (parent_rate <= 0) {
		return parent_rate;
	}

	frg_mult = FIELD_GET(SYSCON_FLEXFRGXCTRL_MULT_MASK, (*config->reg));
	return syscon_clock_frg_calc_rate(parent_rate, frg_mult);
}

static int syscon_clock_frg_configure(const struct clk *clk_hw, const void *mult)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	uint32_t mult_val = FIELD_PREP(SYSCON_FLEXFRGXCTRL_MULT_MASK, ((uint32_t)mult));
	int parent_rate = clock_get_rate(config->parent);
	uint32_t frg_mult = FIELD_GET(SYSCON_FLEXFRGXCTRL_MULT_MASK, (*config->reg));
	uint32_t old_rate, new_rate;
	int ret;

	old_rate = syscon_clock_frg_calc_rate(parent_rate, frg_mult);
	new_rate = syscon_clock_frg_calc_rate(parent_rate, (uint32_t)mult);

	/* Check if consumers can accept rate */
	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}

	/* Notify consumers we will apply rate */
	ret = clock_children_notify_pre_change(clk_hw, old_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	/* DIV field should always be 0xFF */
	(*config->reg) = mult_val | SYSCON_FLEXFRGXCTRL_DIV_MASK;

	/* Notify consumers we changed rate */
	ret = clock_children_notify_post_change(clk_hw, old_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_frg_notify(const struct clk *clk_hw, const struct clk *parent,
			    const struct clock_management_event *event)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	uint32_t frg_mult;
	struct clock_management_event notify_event;

	frg_mult = FIELD_GET(SYSCON_FLEXFRGXCTRL_MULT_MASK, (*config->reg));
	notify_event.type = event->type;
	notify_event.old_rate = syscon_clock_frg_calc_rate(event->old_rate,
							   frg_mult);
	notify_event.new_rate = syscon_clock_frg_calc_rate(event->new_rate,
							   frg_mult);

	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_frg_round_rate(const struct clk *clk_hw,
				       uint32_t rate_req)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	int parent_rate = clock_round_rate(config->parent, rate_req);
	int ret;
	uint32_t mult, new_rate;

	if (parent_rate <= 0) {
		return parent_rate;
	}

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

	new_rate = syscon_clock_frg_calc_rate(parent_rate, mult);
	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}
	return new_rate;
}

static int syscon_clock_frg_set_rate(const struct clk *clk_hw,
				     uint32_t rate_req)
{
	const struct syscon_clock_frg_config *config = clk_hw->hw_data;
	int parent_rate = clock_set_rate(config->parent, rate_req);
	uint32_t mult, mult_val;
	uint32_t current_mult = FIELD_GET(SYSCON_FLEXFRGXCTRL_MULT_MASK, (*config->reg));
	int output_rate, ret, current_rate;

	if (parent_rate <= 0) {
		return parent_rate;
	}

	current_rate = syscon_clock_frg_calc_rate(parent_rate, current_mult);

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

	/* Notify children */
	ret = clock_children_notify_pre_change(clk_hw, current_rate, output_rate);
	if (ret < 0) {
		return ret;
	}
	/* Apply new configuration */
	(*config->reg) = mult_val | SYSCON_FLEXFRGXCTRL_DIV_MASK;

	ret = clock_children_notify_post_change(clk_hw, current_rate, output_rate);
	if (ret < 0) {
		return ret;
	}

	return output_rate;
}
#endif

const struct clock_management_driver_api nxp_syscon_frg_api = {
	.get_rate = syscon_clock_frg_get_rate,
	.configure = syscon_clock_frg_configure,
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	.notify = syscon_clock_frg_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_frg_round_rate,
	.set_rate = syscon_clock_frg_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_frg_config nxp_syscon_frg_##inst = {         \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_frg_##inst,                           \
			     &nxp_syscon_frg_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
