/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

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
	uint32_t cur_div = (*config->reg & div_mask) + config->add_factor;
	uint32_t cur_rate = parent_rate / cur_div;
	uint32_t new_rate = parent_rate / ((uint32_t)div_cfg);
	int ret;

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}

	ret = clock_children_notify_pre_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;

	ret = clock_children_notify_post_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_rtcclk_notify(const struct clk *clk_hw, const struct clk *parent,
				      const struct clock_management_event *event)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_factor = (*config->reg & div_mask) + config->add_factor;
	struct clock_management_event notify_event;

	notify_event.type = event->type;
	notify_event.old_rate = event->old_rate / div_factor;
	notify_event.new_rate = event->new_rate / div_factor;

	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_rtcclk_round_rate(const struct clk *clk_hw,
					  uint32_t rate_req)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate, ret;
	uint32_t div_raw, div_factor;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t parent_req = rate_req * config->add_factor;


	/*
	 * Request a parent rate at the lower end of the frequency range
	 * this RTC divider can handle
	 */
	parent_rate = clock_round_rate(config->parent, parent_req);

	if (parent_rate <= 0) {
		return parent_rate;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = (in_clk - (out_clk * add_factor)) / out_clk
	 */
	div_raw = (parent_rate - parent_req) / rate_req;
	div_factor = (div_raw & div_mask) + config->add_factor;

	ret = clock_children_check_rate(clk_hw, (parent_rate / div_factor));
	if (ret < 0) {
		return ret;
	}

	return parent_rate / div_factor;
}

static int syscon_clock_rtcclk_set_rate(const struct clk *clk_hw,
					uint32_t rate_req)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int parent_rate, ret;
	uint32_t div_raw, div_factor, new_rate;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t curr_div = (*config->reg & div_mask) + config->add_factor;
	uint32_t parent_req = rate_req * config->add_factor;

	parent_rate = clock_set_rate(config->parent, parent_req);

	if (parent_rate <= 0) {
		return parent_rate;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = (in_clk - (out_clk * add_factor)) / out_clk
	 */
	div_raw = (parent_rate - parent_req) / rate_req;
	div_factor = (div_raw & div_mask) + config->add_factor;
	new_rate = parent_rate / div_factor;
	ret = clock_children_notify_pre_change(clk_hw, (parent_rate / curr_div),
					       new_rate);
	if (ret < 0) {
		return ret;
	}
	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;

	ret = clock_children_notify_post_change(clk_hw, (parent_rate / curr_div),
						new_rate);
	if (ret < 0) {
		return ret;
	}

	return new_rate;

}
#endif

const struct clock_management_driver_api nxp_syscon_rtcclk_api = {
	.get_rate = syscon_clock_rtcclk_get_rate,
	.configure = syscon_clock_rtcclk_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = syscon_clock_rtcclk_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
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
