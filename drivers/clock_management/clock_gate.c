/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT clock_gate

struct clock_gate_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	uintptr_t reg;
	uint8_t enable_offset;
};

static clock_freq_t clock_gate_recalc_rate(const struct clk *clk_hw, clock_freq_t parent_rate)
{
	const struct clock_gate_config *config = clk_hw->hw_data;

	return (sys_read32(config->reg) & BIT(config->enable_offset)) ? parent_rate : 0;
}

static int clock_gate_configure(const struct clk *clk_hw, const void *data)
{
	const struct clock_gate_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	if (ungate) {
		sys_write32(sys_read32(config->reg) | BIT(config->enable_offset), config->reg);
	} else {
		sys_write32(sys_read32(config->reg) & ~BIT(config->enable_offset), config->reg);
	}

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t clock_gate_configure_recalc(const struct clk *clk_hw, const void *data,
						clock_freq_t parent_rate)
{
	ARG_UNUSED(clk_hw);

	bool ungate = (bool)data;

	return (ungate) ? parent_rate : 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t clock_gate_best_rate(const struct clk *clk_hw, clock_freq_t rate_req,
					 clock_freq_t parent_rate, bool commit)
{
	if (commit) {
		const struct clock_gate_config *config = clk_hw->hw_data;

		if (rate_req != 0) {
			sys_write32(sys_read32(config->reg) | BIT(config->enable_offset),
				    config->reg);
		} else {
			sys_write32(sys_read32(config->reg) & ~BIT(config->enable_offset),
				    config->reg);
		}
	}
	return (rate_req != 0) ? parent_rate : 0;
}
#endif

const struct clock_management_standard_api clock_gate_api = {
	.recalc_rate = clock_gate_recalc_rate,
	.shared.configure = clock_gate_configure,
#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
	.configure_recalc = clock_gate_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.best_rate = clock_gate_best_rate,
#endif
};

#define CLOCK_GATE_DEFINE(inst)                                                                    \
	const struct clock_gate_config clock_gate_##inst = {                                       \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PHANDLE(inst, input))).reg =    \
			DT_INST_REG_ADDR(inst),                                                    \
		.enable_offset = (uint8_t)DT_INST_PROP(inst, offset),                              \
	};                                                                                         \
                                                                                                   \
	CLOCK_DT_INST_DEFINE(inst, &clock_gate_##inst, &clock_gate_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_GATE_DEFINE)
