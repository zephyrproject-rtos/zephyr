/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>
#include <soc.h>

#define DT_DRV_COMPAT nxp_syscon_clock_source

struct syscon_clock_source_config {
	uint8_t enable_offset;
	uint32_t pdown_mask:24;
	uint32_t rate;
	volatile uint32_t *reg;
};

static int syscon_clock_source_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	return ((*config->reg) & BIT(config->enable_offset)) ?
		config->rate : 0;
}

static int syscon_clock_source_configure(const struct clk *clk_hw, const void *data)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	int ret;
	bool ungate = (bool)data;

	if (ungate) {
		ret = clock_notify_children(clk_hw, config->rate);
		if (ret < 0) {
			return ret;
		}
		(*config->reg) |= BIT(config->enable_offset);
		PMC->PDRUNCFGCLR0 = config->pdown_mask;
	} else {
		ret = clock_notify_children(clk_hw, 0);
		if (ret  < 0) {
			return ret;
		}
		(*config->reg) &= ~BIT(config->enable_offset);
		PMC->PDRUNCFGSET0 = config->pdown_mask;
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int syscon_clock_source_notify(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	int ret;

	ret = clock_notify_children(clk_hw, clock_get_rate(clk_hw));
	if (ret == CLK_NO_CHILDREN) {
		/* Notify children we are going to gate clock source */
		ret = clock_notify_children(clk_hw, 0);
		if (ret == CLK_NO_CHILDREN) {
			/* Gate this clock source */
			(*config->reg) &= ~BIT(config->enable_offset);
			PMC->PDRUNCFGSET0 = config->pdown_mask;
		}
	}

	return 0;
}
#endif


#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int syscon_clock_source_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	return (rate != 0) ? config->rate : 0;
}

static int syscon_clock_source_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;

	/* If the clock rate is 0, gate the source */
	if (rate == 0) {
		syscon_clock_source_configure(clk_hw, (void *)0);
	} else {
		syscon_clock_source_configure(clk_hw, (void *)1);
	}

	return (rate != 0) ? config->rate : 0;
}
#endif

const struct clock_driver_api nxp_syscon_source_api = {
	.get_rate = syscon_clock_source_get_rate,
	.configure = syscon_clock_source_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_clock_source_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = syscon_clock_source_round_rate,
	.set_rate = syscon_clock_source_set_rate,
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
