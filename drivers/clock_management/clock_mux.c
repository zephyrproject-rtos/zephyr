/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>

#define DT_DRV_COMPAT clock_mux

struct clock_mux_config {
	MUX_CLK_SUBSYS_DATA_DEFINE
	uintptr_t reg;
	uint8_t mask_width;
	uint8_t mask_offset;
};

static int mux_configure(const struct clk *clk_hw, const void *mux)
{
	const struct clock_mux_config *config = clk_hw->hw_data;

	return clock_management_mux_set_parent((uintptr_t)config->reg, config->mask_width,
					       config->mask_offset, config->parent_cnt,
					       (uint32_t)(uintptr_t)mux);
}

static int mux_get_parent(const struct clk *clk_hw)
{
	const struct clock_mux_config *config = clk_hw->hw_data;

	return clock_management_mux_get_parent((uintptr_t)config->reg, config->mask_width,
					       config->mask_offset, config->parent_cnt);
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int mux_configure_recalc(const struct clk *clk_hw, const void *mux)
{
	const struct clock_mux_config *config = clk_hw->hw_data;

	return clock_management_mux_validate_parent(config->parent_cnt, (uint32_t)(uintptr_t)mux);
}

static int mux_validate_parent(const struct clk *clk_hw, clock_freq_t parent_freq, uint8_t new_idx)
{
	/* This mux is simple and will switch regardless of the parent frequency */
	ARG_UNUSED(parent_freq);
	const struct clock_mux_config *config = clk_hw->hw_data;

	return clock_management_mux_validate_parent(config->parent_cnt, (uint32_t)new_idx);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int mux_set_parent(const struct clk *clk_hw, uint8_t new_idx)
{
	return mux_configure(clk_hw, (const void *)(uintptr_t)new_idx);
}
#endif

const struct clock_management_mux_api mux_api = {
	.shared.configure = mux_configure,
	.get_parent = mux_get_parent,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.mux_configure_recalc = mux_configure_recalc,
	.mux_validate_parent = mux_validate_parent,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.set_parent = mux_set_parent,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx) CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define CLOCK_MUX_DEFINE(inst)                                                                     \
	static const struct clk *const mux_##inst##_parents[] = {                                  \
		DT_INST_FOREACH_PROP_ELEM(inst, input_sources, GET_MUX_INPUT)};                    \
	static const struct clock_mux_config mux_##inst = {                                        \
		MUX_CLK_SUBSYS_DATA_INIT(mux_##inst##_parents,                                     \
					 DT_INST_PROP_LEN(inst, input_sources))                    \
			.reg = DT_INST_REG_ADDR(inst),                                             \
		.mask_width = (uint8_t)DT_INST_REG_SIZE(inst),                                     \
		.mask_offset = (uint8_t)DT_INST_PROP(inst, offset),                                \
	};                                                                                         \
	MUX_CLOCK_DT_INST_DEFINE(inst, &mux_##inst, &mux_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_MUX_DEFINE)
