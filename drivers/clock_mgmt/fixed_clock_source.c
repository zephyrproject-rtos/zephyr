/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>
#include <soc.h>

#define DT_DRV_COMPAT fixed_clock_source

static int clock_source_get_rate(const struct clk *clk_hw)
{
	return (int)clk_hw->hw_data;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int clock_source_notify(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
	return clock_notify_children(clk_hw, (int)clk_hw->hw_data);
}
#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)

static int clock_source_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	return (int)clk_hw->hw_data;
}

static int clock_source_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	return (int)clk_hw->hw_data;
}

#endif

const struct clock_driver_api clock_source_api = {
	.get_rate = clock_source_get_rate,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = clock_source_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = clock_source_round_rate,
	.set_rate = clock_source_set_rate,
#endif
};

#define CLOCK_SOURCE_DEFINE(inst)                                              \
	ROOT_CLOCK_DT_INST_DEFINE(inst,                                        \
				  ((uint32_t *)DT_INST_PROP(inst, frequency)), \
				  &clock_source_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_SOURCE_DEFINE)
