/*
 * Copyright 2024 NXP
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
	int current_rate = clock_get_rate(clk_hw);

	if (ungate) {
		/* Check if children will accept this rate */
		ret = clock_children_check_rate(clk_hw, config->rate);
		if (ret < 0) {
			return ret;
		}
		ret = clock_children_notify_pre_change(clk_hw, current_rate,
						       config->rate);
		if (ret < 0) {
			return ret;
		}
		(*config->reg) |= BIT(config->enable_offset);
		PMC->PDRUNCFGCLR0 = config->pdown_mask;
		return clock_children_notify_post_change(clk_hw, current_rate,
							 config->rate);
	}
	/* Check if children will accept this rate */
	ret = clock_children_check_rate(clk_hw, 0);
	if (ret < 0) {
		return ret;
	}
	/* Pre rate change notification */
	ret = clock_children_notify_pre_change(clk_hw, current_rate, 0);
	if (ret < 0) {
		return ret;
	}
	(*config->reg) &= ~BIT(config->enable_offset);
	PMC->PDRUNCFGSET0 = config->pdown_mask;
	return clock_children_notify_post_change(clk_hw, current_rate, 0);
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_source_notify(const struct clk *clk_hw, const struct clk *parent,
			       const struct clock_management_event *event)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	int ret;
	int clock_rate = clock_get_rate(clk_hw);
	const struct clock_management_event notify_event = {
		/*
		 * Use QUERY type, no need to forward this notification to
		 * consumers
		 */
		.type = CLOCK_MANAGEMENT_QUERY_RATE_CHANGE,
		.old_rate = clock_rate,
		.new_rate = clock_rate,
	};

	ARG_UNUSED(event);
	ret = clock_notify_children(clk_hw, &notify_event);
	if (ret == CLK_NO_CHILDREN) {
		/* Gate this clock source */
		(*config->reg) &= ~BIT(config->enable_offset);
		PMC->PDRUNCFGSET0 = config->pdown_mask;
	}

	return 0;
}
#endif


#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_source_round_rate(const struct clk *clk_hw,
					  uint32_t rate_req)
{
	const struct syscon_clock_source_config *config = clk_hw->hw_data;
	int ret;

	if (rate_req != 0) {
		ret = clock_children_check_rate(clk_hw, config->rate);
		if (ret >= 0) {
			return config->rate;
		}
	} else {
		ret = clock_children_check_rate(clk_hw, 0);
		if (ret >= 0) {
			return 0;
		}
	}
	/* Rate was not accepted */
	return -ENOTSUP;
}

static int syscon_clock_source_set_rate(const struct clk *clk_hw,
					uint32_t rate_req)
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

const struct clock_management_driver_api nxp_syscon_source_api = {
	.get_rate = syscon_clock_source_get_rate,
	.configure = syscon_clock_source_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = syscon_clock_source_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
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
