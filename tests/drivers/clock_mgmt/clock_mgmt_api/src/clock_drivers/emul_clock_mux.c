/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>
#include <stdlib.h>

#define DT_DRV_COMPAT vnd_emul_clock_mux

struct emul_clock_mux {
	uint8_t src_count;
	uint8_t src_sel;
	const struct clk *parents[];
};

static int emul_clock_mux_get_rate(const struct clk *clk_hw)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	return clock_get_rate(data->parents[data->src_sel]);
}

static int emul_clock_mux_configure(const struct clk *clk_hw, const void *mux)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	int ret;

	if (((uint32_t)mux) > data->src_count) {
		return -EINVAL;
	}

	ret = clock_notify_children(clk_hw, clock_get_rate(data->parents[(uint32_t)mux]));
	if (ret < 0) {
		return ret;
	}

	/* Apply source selection */
	data->src_sel = (uint32_t)mux;
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int emul_clock_mux_notify(const struct clk *clk_hw, const struct clk *parent,
				 uint32_t parent_rate)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	/*
	 * Read selector, and if index matches parent index we should notify
	 * children
	 */
	if (data->parents[data->src_sel] == parent) {
		return clock_notify_children(clk_hw, parent_rate);
	}

	/* Parent is not in use */
	return -ENOTCONN;
}
#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)

static int emul_clock_mux_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	int cand_rate;
	int target_rate = rate;
	int best_delta = INT32_MAX;
	int best_rate = 0;
	uint8_t idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < data->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(data->parents[idx], rate, clk_hw);
		if (abs(cand_rate - target_rate) < best_delta) {
			best_rate = cand_rate;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	return best_rate;
}

static int emul_clock_mux_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	int cand_rate, best_rate, ret, target_rate;
	int best_delta = INT32_MAX;
	uint8_t idx = 0;
	uint8_t best_idx = 0;

	target_rate = rate;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < data->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(data->parents[idx], rate, clk_hw);
		if (abs(cand_rate - target_rate) < best_delta) {
			best_idx = idx;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	/* Now set the clock rate for the best parent */
	best_rate = clock_set_rate(data->parents[best_idx], rate, clk_hw);
	if (best_rate < 0) {
		return best_rate;
	}
	ret = clock_notify_children(clk_hw, best_rate);
	if (ret < 0) {
		return ret;
	}
	/* Unlock the previous parent, so it can be reconfigured */
	clock_unlock(data->parents[data->src_sel], clk_hw);
	/* Set new parent selector */
	data->src_sel = best_idx;

	return best_rate;
}
#endif

const struct clock_driver_api emul_mux_api = {
	.get_rate = emul_clock_mux_get_rate,
	.configure = emul_clock_mux_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = emul_clock_mux_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = emul_clock_mux_round_rate,
	.set_rate = emul_clock_mux_set_rate,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx)                                      \
	CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define EMUL_CLOCK_DEFINE(inst)                                                \
	struct emul_clock_mux emul_clock_mux_##inst = {                        \
		.src_count = DT_INST_PROP_LEN(inst, inputs),                   \
		.parents = {                                                   \
			DT_INST_FOREACH_PROP_ELEM(inst, inputs,                \
						GET_MUX_INPUT)                 \
		},                                                             \
		.src_sel = 0,                                                  \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &emul_clock_mux_##inst,                           \
			     &emul_mux_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_CLOCK_DEFINE)
