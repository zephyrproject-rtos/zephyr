/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT nxp_syscon_clock_gate

struct syscon_clock_gate_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	volatile uint32_t *reg;
	uint8_t enable_offset;
};

static clock_freq_t syscon_clock_gate_recalc_rate(const struct clk *clk_hw,
					clock_freq_t parent_rate)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	return ((*config->reg) & BIT(config->enable_offset)) ?
		parent_rate : 0;
}

static int syscon_clock_gate_configure(const struct clk *clk_hw, const void *data)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	if (ungate) {
		(*config->reg) |= BIT(config->enable_offset);
	} else {
		(*config->reg) &= ~BIT(config->enable_offset);
	}

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t syscon_clock_gate_configure_recalc(const struct clk *clk_hw,
					      const void *data,
					      clock_freq_t parent_rate)
{
	bool ungate = (bool)data;

	return (ungate) ? parent_rate : 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t syscon_clock_gate_round_rate(const struct clk *clk_hw,
					clock_freq_t rate_req,
					clock_freq_t parent_rate)
{
	if (rate_req != 0) {
		return parent_rate;
	}
	return 0;
}

static clock_freq_t syscon_clock_gate_set_rate(const struct clk *clk_hw,
				      clock_freq_t rate_req,
				      clock_freq_t parent_rate)
{
	const struct syscon_clock_gate_config *config = clk_hw->hw_data;

	if (rate_req != 0) {
		(*config->reg) |= BIT(config->enable_offset);
		return parent_rate;
	}
	(*config->reg) &= ~BIT(config->enable_offset);
	return 0;
}
#endif

const struct clock_management_standard_api nxp_syscon_gate_api = {
	.recalc_rate = syscon_clock_gate_recalc_rate,
	.shared.configure = syscon_clock_gate_configure,
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	.configure_recalc = syscon_clock_gate_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_gate_round_rate,
	.set_rate = syscon_clock_gate_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_gate_config nxp_syscon_gate_##inst = {       \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.enable_offset = (uint8_t)DT_INST_PROP(inst, offset),          \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_gate_##inst,                          \
			     &nxp_syscon_gate_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
