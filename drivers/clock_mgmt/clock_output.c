/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>

#define DT_DRV_COMPAT clock_output

static int clock_output_get_rate(const struct clk *clk_hw)
{
	const struct clk *parent = (const struct clk *)clk_hw->hw_data;

	return clock_get_rate(parent);
}

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int clock_output_configure(const struct clk *clk_hw, const void *rate)
{
	const struct clk *parent = (const struct clk *)clk_hw->hw_data;
	int ret;

	ret =  clock_set_rate(parent, (uint32_t)rate, clk_hw);
	if (ret < 0) {
		return ret;
	}
	return 0;
}
#endif

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int clock_output_notify(const struct clk *clk_hw, const struct clk *parent,
			       uint32_t parent_rate)
{
	return clock_notify_children(clk_hw, parent_rate);
}
#endif

const struct clock_driver_api clock_output_api = {
	.get_rate = clock_output_get_rate,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = clock_output_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.configure = clock_output_configure,
#endif
};

#define CLOCK_OUTPUT_DEFINE(inst)                                              \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     CLOCK_DT_GET(DT_INST_PARENT(inst)),               \
			     &clock_output_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_DEFINE)
