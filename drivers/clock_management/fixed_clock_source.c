/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT fixed_clock

struct fixed_clock_data {
	int clock_rate;
};

static int clock_source_get_rate(const struct clk *clk_hw)
{
	return ((struct fixed_clock_data *)clk_hw->hw_data)->clock_rate;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int clock_source_notify(const struct clk *clk_hw, const struct clk *parent,
			       const struct clock_management_event *event)
{
	const struct fixed_clock_data *data = clk_hw->hw_data;
	const struct clock_management_event notify_event = {
		/*
		 * Use QUERY type, no need to forward this notification to
		 * consumers
		 */
		.type = CLOCK_MANAGEMENT_QUERY_RATE_CHANGE,
		.old_rate = data->clock_rate,
		.new_rate = data->clock_rate,
	};

	ARG_UNUSED(event);
	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

static int clock_source_round_rate(const struct clk *clk_hw, uint32_t rate_req)
{
	const struct fixed_clock_data *data = clk_hw->hw_data;

	return data->clock_rate;
}

static int clock_source_set_rate(const struct clk *clk_hw, uint32_t rate_req)
{
	const struct fixed_clock_data *data = clk_hw->hw_data;

	return data->clock_rate;
}

#endif

const struct clock_management_driver_api fixed_clock_source_api = {
	.get_rate = clock_source_get_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = clock_source_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = clock_source_round_rate,
	.set_rate = clock_source_set_rate,
#endif
};

#define FIXED_CLOCK_SOURCE_DEFINE(inst)                                        \
	const struct fixed_clock_data fixed_clock_data_##inst = {              \
		.clock_rate = DT_INST_PROP(inst, clock_frequency),             \
	};                                                                     \
	ROOT_CLOCK_DT_INST_DEFINE(inst,                                        \
				  &fixed_clock_data_##inst,                    \
				  &fixed_clock_source_api);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLOCK_SOURCE_DEFINE)
