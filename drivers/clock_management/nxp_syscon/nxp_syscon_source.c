/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <soc.h>

#define DT_DRV_COMPAT nxp_syscon_clock_source

struct syscon_clock_source_config {
	uint8_t enable_offset;
	uint32_t pdown_mask:24;
	uint32_t rate;
	volatile uint32_t *reg;
};

static clock_freq_t syscon_clock_source_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	return ((*config->reg) & BIT(config->enable_offset)) ?
		config->rate : 0;
}

static int syscon_clock_source_configure(const struct clk *clk_hw, const void *data)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;
	volatile uint32_t *power_reg;
	uint32_t pdown_val;

	if (ungate) {
		power_reg = &PMC->PDRUNCFGCLR0;
		pdown_val = (*config->reg) | BIT(config->enable_offset);
	} else {
		power_reg = &PMC->PDRUNCFGSET0;
		pdown_val = (*config->reg) & ~BIT(config->enable_offset);
	}
	(*config->reg) = pdown_val;
	*power_reg = config->pdown_mask;
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t syscon_clock_source_configure_recalc(const struct clk *clk_hw,
					       const void *data)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	bool ungate = (bool)data;

	return ungate ? config->rate : 0;
}
#endif


#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t syscon_clock_source_round_rate(const struct clk *clk_hw,
					  clock_freq_t rate_req)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	return (rate_req != 0) ? config->rate : 0;
}

static clock_freq_t syscon_clock_source_set_rate(const struct clk *clk_hw,
					clock_freq_t rate_req)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	/* If the clock rate is 0, gate the source */
	if (rate_req == 0) {
		syscon_clock_source_configure(clk_hw, (void *)0);
	} else {
		syscon_clock_source_configure(clk_hw, (void *)1);
	}
	return (rate_req != 0) ? config->rate : 0;
}
#endif

const struct clock_management_root_api nxp_syscon_source_api = {
	.get_rate = syscon_clock_source_get_rate,
	.shared.configure = syscon_clock_source_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.root_configure_recalc = syscon_clock_source_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.root_round_rate = syscon_clock_source_round_rate,
	.root_set_rate = syscon_clock_source_set_rate,
#endif
};

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_source_config nxp_syscon_source_##inst = {   \
		.rate = DT_INST_PROP(inst, frequency),                         \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.enable_offset = (uint8_t)DT_INST_PROP(inst, offset),          \
		.pdown_mask = DT_INST_PROP(inst, pdown_mask),                  \
	};                                                                     \
	                                                                       \
	ROOT_CLOCK_DT_INST_DEFINE(inst,                                        \
				  &nxp_syscon_source_##inst,                   \
				  &nxp_syscon_source_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
