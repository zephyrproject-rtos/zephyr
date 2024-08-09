/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>

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
	int ret;
	bool ungate = (bool)data;

	if (ungate) {
		ret = clock_notify_children(clk_hw, clock_get_rate(config->parent));
		if (ret < 0) {
			return ret;
		}
		(*config->reg) |= BIT(config->enable_offset);
	} else {
		ret = clock_notify_children(clk_hw, 0);
		if (ret < 0) {
			return ret;
		}
		(*config->reg) &= ~BIT(config->enable_offset);
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int syscon_clock_gate_notify(const struct clk *clk_hw, const struct clk *parent,
			     uint32_t parent_rate)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	if ((*config->reg) & BIT(config->enable_offset)) {
		return clock_notify_children(clk_hw, parent_rate);
	}
	/* Clock is gated */
	return clock_notify_children(clk_hw, 0);
}
#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int syscon_clock_gate_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	return (rate != 0) ? clock_round_rate(config->parent, rate, clk_hw) : 0;
}

static int syscon_clock_gate_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	/* If the clock rate is 0, gate the source */
	if (rate == 0) {
		syscon_clock_gate_configure(clk_hw, (void *)0);
	} else {
		syscon_clock_gate_configure(clk_hw, (void *)1);
	}

	return (rate != 0) ? clock_set_rate(config->parent, rate, clk_hw) : 0;
}
#endif

const struct clock_driver_api nxp_syscon_gate_api = {
	.get_rate = syscon_clock_gate_get_rate,
	.configure = syscon_clock_gate_configure,
#ifdef CONFIG_CLOCK_MGMT_NOTIFY
	.notify = syscon_clock_gate_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
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
