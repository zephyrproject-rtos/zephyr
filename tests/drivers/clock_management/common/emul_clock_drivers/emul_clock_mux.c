/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <stdlib.h>

#define DT_DRV_COMPAT vnd_emul_clock_mux

struct emul_clock_mux {
	MUX_CLK_SUBSYS_DATA_DEFINE
	uint8_t src_count;
	uint8_t src_sel;
};

static int emul_clock_mux_get_parent(const struct clk *clk_hw)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	return data->src_sel;
}

static int emul_clock_mux_configure(const struct clk *clk_hw, const void *mux)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	uint32_t mux_val = (uint32_t)(uintptr_t)mux;

	if (mux_val > data->src_count) {
		return -EINVAL;
	}

	/* Apply source selection */
	data->src_sel = mux_val;

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int emul_clock_mux_configure_recalc(const struct clk *clk_hw,
					       const void *mux)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	uint32_t mux_val = (uint32_t)(uintptr_t)mux;

	if (mux_val > data->src_count) {
		return -EINVAL;
	}

	return mux_val;
}

static int emul_clock_mux_validate_parent(const struct clk *clk_hw,
					  clock_freq_t parent_freq,
					  uint8_t new_idx)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	if (new_idx >= data->src_count) {
		return -EINVAL;
	}

	/* For emulated clock, we assume all parents are valid */
	return 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

static int emul_clock_mux_set_parent(const struct clk *clk_hw,
				     uint8_t new_idx)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	if (new_idx >= data->src_count) {
		return -ENOENT;
	}

	data->src_sel = new_idx;

	return 0;
}
#endif

const struct clock_management_mux_api emul_mux_api = {
	.shared.configure = emul_clock_mux_configure,
	.get_parent = emul_clock_mux_get_parent,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.mux_configure_recalc = emul_clock_mux_configure_recalc,
	.mux_validate_parent = emul_clock_mux_validate_parent,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.set_parent = emul_clock_mux_set_parent,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx)                                      \
	CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define EMUL_CLOCK_DEFINE(inst)                                                \
	const struct clk *const emul_clock_mux_parents_##inst[] = {            \
		DT_INST_FOREACH_PROP_ELEM(inst, inputs, GET_MUX_INPUT)         \
	};                                                                     \
	struct emul_clock_mux emul_clock_mux_##inst = {                        \
		MUX_CLK_SUBSYS_DATA_INIT(emul_clock_mux_parents_##inst,        \
					 DT_INST_PROP_LEN(inst, inputs))       \
		.src_count = DT_INST_PROP_LEN(inst, inputs),                   \
		.src_sel = 0,                                                  \
	};                                                                     \
	                                                                       \
	MUX_CLOCK_DT_INST_DEFINE(inst,                                         \
			     &emul_clock_mux_##inst,                           \
			     &emul_mux_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_CLOCK_DEFINE)
