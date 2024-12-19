/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_clock_gate

struct syscon_clock_gate_config {
	const struct clk *parent;
	volatile uint32_t *reg;
	uint8_t enable_offset;
};

static int syscon_clock_gate_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	return ((*config->reg) & BIT(config->enable_offset)) ?
		clock_get_rate(config->parent) : 0;
}

static int syscon_clock_gate_configure(const struct clk *clk_hw, const void *data)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	int cur_rate = ((*config->reg)  & BIT(config->enable_offset)) ?
		parent_rate : 0;
	int ret;
	bool ungate = (bool)data;

	if (ungate) {
		ret = clock_children_check_rate(clk_hw, parent_rate);
		if (ret < 0) {
			return ret;
		}
		ret = clock_children_notify_pre_change(clk_hw, cur_rate,
						       parent_rate);
		if (ret < 0) {
			return ret;
		}
		(*config->reg) |= BIT(config->enable_offset);
		ret = clock_children_notify_post_change(clk_hw, cur_rate,
							parent_rate);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = clock_children_check_rate(clk_hw, 0);
		if (ret < 0) {
			return ret;
		}
		ret = clock_children_notify_pre_change(clk_hw, cur_rate, 0);
		if (ret < 0) {
			return ret;
		}
		(*config->reg) &= ~BIT(config->enable_offset);
		ret = clock_children_notify_post_change(clk_hw, cur_rate, 0);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_gate_notify(const struct clk *clk_hw, const struct clk *parent,
				    const struct clock_management_event *event)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;
	struct clock_management_event notify_event;

	notify_event.type = event->type;
	if ((*config->reg) & BIT(config->enable_offset)) {
		notify_event.old_rate = event->old_rate;
		notify_event.new_rate = event->new_rate;
	} else {
		/* Clock is gated */
		notify_event.old_rate = event->old_rate;
		notify_event.new_rate = 0;
	}
	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_gate_round_rate(const struct clk *clk_hw,
					uint32_t rate_req)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;
	int new_rate = (rate_req != 0) ?
		clock_round_rate(config->parent, rate_req) : 0;
	int ret;

	if (new_rate < 0) {
		return new_rate;
	}

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}

	return new_rate;
}

static int syscon_clock_gate_set_rate(const struct clk *clk_hw,
				      uint32_t rate_req)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;
	int ret, new_rate;

	if (rate_req != 0) {
		new_rate = clock_set_rate(config->parent, rate_req);
		if (new_rate < 0) {
			return new_rate;
		}
		ret = syscon_clock_gate_configure(clk_hw, (void *)1);
		if (ret < 0) {
			return ret;
		}
		return new_rate;
	}
	/* Gate the source */
	ret = syscon_clock_gate_configure(clk_hw, (void *)1);
	if (ret < 0) {
		return ret;
	}
	return 0;
}
#endif

const struct clock_management_driver_api nxp_syscon_gate_api = {
	.get_rate = syscon_clock_gate_get_rate,
	.configure = syscon_clock_gate_configure,
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	.notify = syscon_clock_gate_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_gate_round_rate,
	.set_rate = syscon_clock_gate_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_gate_config nxp_syscon_gate_##inst = {       \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.enable_offset = (uint8_t)DT_INST_PROP(inst, offset),          \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_gate_##inst,                          \
			     &nxp_syscon_gate_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
