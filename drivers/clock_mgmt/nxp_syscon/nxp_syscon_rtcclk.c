/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_rtcclk

struct syscon_rtcclk_config {
	uint16_t add_factor;
	uint8_t mask_offset;
	uint8_t mask_width;
	const struct clk *parent;
	volatile uint32_t *reg;
};


static int syscon_clock_rtcclk_get_rate(const struct clk *clk_hw)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_factor = (*config->reg & div_mask) + config->add_factor;

	if (parent_rate <= 0) {
		return parent_rate;
	}

	/* Calculate divided clock */
	return parent_rate / div_factor;
}

static int syscon_clock_rtcclk_configure(const struct clk *clk_hw, const void *div_cfg)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_val = ((uint32_t)div_cfg) - config->add_factor;
	uint32_t div_raw = FIELD_PREP(div_mask, div_val);
	uint32_t new_rate = parent_rate / ((uint32_t)div_cfg);
	int ret;

	ret = clock_notify_children(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}
	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int syscon_clock_rtcclk_notify(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_factor = (*config->reg & div_mask) + config->add_factor;


	return clock_notify_children(clk_hw, (parent_rate / div_factor));
}
#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int syscon_clock_rtcclk_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate;
	uint32_t div_raw, div_factor;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);

	/*
	 * Request a parent rate at the lower end of the frequency range
	 * this RTC divider can handle
	 */
	parent_rate = clock_round_rate(config->parent,
				       rate * config->add_factor, clk_hw);

	if (parent_rate <= 0) {
		return parent_rate;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = (in_clk - (out_clk * add_factor)) / out_clk
	 */
	div_raw = (parent_rate - (rate * config->add_factor)) / rate;
	div_factor = (div_raw & div_mask) + config->add_factor;

	return parent_rate / div_factor;
}

static int syscon_clock_rtcclk_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate, ret;
	uint32_t div_raw, div_factor, new_rate;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);

	/*
	 * Request a parent rate at the lower end of the frequency range
	 * this RTC divider can handle
	 */
	parent_rate = clock_set_rate(config->parent,
				     rate * config->add_factor, clk_hw);

	if (parent_rate <= 0) {
		return parent_rate;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = (in_clk - (out_clk * add_factor)) / out_clk
	 */
	div_raw = (parent_rate - (rate * config->add_factor)) / rate;
	div_factor = (div_raw & div_mask) + config->add_factor;
	new_rate = parent_rate / div_factor;
	ret = clock_notify_children(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}
	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;

	return new_rate;

}
#endif

const struct clock_driver_api nxp_syscon_rtcclk_api = {
	.get_rate = syscon_clock_rtcclk_get_rate,
	.configure = syscon_clock_rtcclk_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_clock_rtcclk_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = syscon_clock_rtcclk_round_rate,
	.set_rate = syscon_clock_rtcclk_set_rate,
#endif
};

#define NXP_RTCCLK_DEFINE(inst)                                                \
	const struct syscon_rtcclk_config nxp_syscon_rtcclk_##inst = {         \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.mask_width = DT_INST_REG_SIZE(inst),                          \
		.mask_offset = DT_INST_PROP(inst, offset),                     \
		.add_factor = DT_INST_PROP(inst, add_factor),                  \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_rtcclk_##inst,                        \
			     &nxp_syscon_rtcclk_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_RTCCLK_DEFINE)
