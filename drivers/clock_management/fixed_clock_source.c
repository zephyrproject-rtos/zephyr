/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT fixed_clock

struct fixed_clock_data {
	clock_freq_t clock_rate;
};

static clock_freq_t clock_source_get_rate(const struct clk *clk_hw)
{
	return ((struct fixed_clock_data *)clk_hw->hw_data)->clock_rate;
}

#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE
static clock_freq_t clock_source_request_rate(const struct clk *clk_hw, clock_freq_t rate_req)
{
	/* Clock isn't reconfigurable, just return current rate */
	ARG_UNUSED(rate_req);
	return ((struct fixed_clock_data *)clk_hw->hw_data)->clock_rate;
}
#endif

const struct clock_management_root_api fixed_clock_source_api = {
	.get_rate = clock_source_get_rate,
#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE
	.root_round_rate = clock_source_request_rate,
	.root_set_rate = clock_source_request_rate,
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
