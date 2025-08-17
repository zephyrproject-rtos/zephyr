/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT clock_source

struct clock_source_config {
	uint32_t rate;
	volatile uint32_t *reg;
	uint8_t gate_offset;
};

static clock_freq_t clock_source_get_rate(const struct clk *clk_hw)
{
	const struct clock_source_config *config = clk_hw->hw_data;

	return ((*config->reg) & BIT(config->gate_offset)) ?
		config->rate : 0;
}

static int clock_source_configure(const struct clk *clk_hw, const void *data)
{
	const struct clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	if (ungate) {
		(*config->reg) |= BIT(config->gate_offset);
	} else {
		(*config->reg) &= ~BIT(config->gate_offset);
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t clock_source_configure_recalc(const struct clk *clk_hw,
						  const void *data)
{
	const struct clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	return ungate ? config->rate : 0;
}
#endif


#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t clock_source_round_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	const struct clock_source_config *config = clk_hw->hw_data;

	return (rate_req != 0) ? config->rate : 0;
}

static clock_freq_t clock_source_set_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	const struct clock_source_config *config = clk_hw->hw_data;

	/* If the clock rate is 0, gate the source */
	if (rate_req == 0) {
		clock_source_configure(clk_hw, (void *)0);
	} else {
		clock_source_configure(clk_hw, (void *)1);
	}

	return (rate_req != 0) ? config->rate : 0;
}
#endif

const struct clock_management_root_api clock_source_api = {
	.get_rate = clock_source_get_rate,
	.shared.configure = clock_source_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.root_configure_recalc = clock_source_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.root_round_rate = clock_source_round_rate,
	.root_set_rate = clock_source_set_rate,
#endif
};

#define CLOCK_SOURCE_DEFINE(inst)                                              \
	const struct clock_source_config clock_source_##inst = {               \
		.rate = DT_INST_PROP(inst, clock_frequency),                   \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.gate_offset = (uint8_t)DT_INST_PROP(inst, gate_offset),       \
	};                                                                     \
	                                                                       \
	ROOT_CLOCK_DT_INST_DEFINE(inst,                                        \
				  &clock_source_##inst,                        \
				  &clock_source_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_SOURCE_DEFINE)
