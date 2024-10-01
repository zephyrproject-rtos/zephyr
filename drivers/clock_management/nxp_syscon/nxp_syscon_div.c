/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_clock_div

struct syscon_clock_div_config {
	uint8_t mask_width;
	const struct clk *parent;
	volatile uint32_t *reg;
};


static int syscon_clock_div_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_div_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	uint8_t div_mask = GENMASK((config->mask_width - 1), 0);

	if (parent_rate <= 0) {
		return parent_rate;
	}

	/* Calculate divided clock */
	return parent_rate / ((*config->reg & div_mask) + 1);
}

static int syscon_clock_div_configure(const struct clk *clk_hw, const void *div_cfg)
{
	const struct syscon_clock_div_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width - 1), 0);
	uint32_t cur_div = ((*config->reg & div_mask) + 1);
	uint32_t div_val = (((uint32_t)div_cfg) - 1) & div_mask;
	int parent_rate = clock_get_rate(config->parent);
	uint32_t new_rate = (parent_rate / ((uint32_t)div_cfg));
	uint32_t cur_rate = (parent_rate / cur_div);
	int ret;

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}
	ret = clock_children_notify_pre_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	(*config->reg) = ((*config->reg) & ~div_mask) | div_val;

	ret = clock_children_notify_post_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_div_notify(const struct clk *clk_hw, const struct clk *parent,
				   const struct clock_management_event *event)
{
	const struct syscon_clock_div_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width - 1), 0);
	struct clock_management_event notify_event;

	notify_event.type = event->type;
	notify_event.old_rate =
		(event->old_rate / ((*config->reg & div_mask) + 1));
	notify_event.new_rate =
		(event->new_rate / ((*config->reg & div_mask) + 1));

	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_div_round_rate(const struct clk *clk_hw,
				       uint32_t rate_req)
{
	const struct syscon_clock_div_config *config = clk_hw->hw_data;
	int parent_rate = clock_round_rate(config->parent, rate_req);
	int div_val = MAX((parent_rate / rate_req), 1) - 1;
	uint8_t div_mask = GENMASK((config->mask_width - 1), 0);
	uint32_t new_rate = parent_rate / ((div_val & div_mask) + 1);
	int ret;

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}
	return new_rate;
}

static int syscon_clock_div_set_rate(const struct clk *clk_hw,
				     uint32_t rate_req)
{
	const struct syscon_clock_div_config *config = clk_hw->hw_data;
	int parent_rate = clock_set_rate(config->parent, rate_req);
	int div_val = MAX((parent_rate / rate_req), 1) - 1;
	uint8_t div_mask = GENMASK((config->mask_width - 1), 0);
	uint32_t cur_div = ((*config->reg & div_mask) + 1);
	uint32_t cur_rate = (parent_rate / cur_div);
	uint32_t new_rate = parent_rate / ((div_val & div_mask) + 1);
	int ret;

	ret = clock_children_notify_pre_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	(*config->reg) = ((*config->reg) & ~div_mask) | (div_val & div_mask);

	ret = clock_children_notify_post_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	return new_rate;
}
#endif

const struct clock_management_driver_api nxp_syscon_div_api = {
	.get_rate = syscon_clock_div_get_rate,
	.configure = syscon_clock_div_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = syscon_clock_div_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_div_round_rate,
	.set_rate = syscon_clock_div_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_div_config nxp_syscon_div_##inst = {         \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.mask_width = (uint8_t)DT_INST_REG_SIZE(inst),                 \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_div_##inst,                           \
			     &nxp_syscon_div_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
