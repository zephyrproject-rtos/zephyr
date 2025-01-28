/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
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
	int curr_rate = clock_get_rate(clk_hw);
	int new_rate;
	int ret;

	if (((uint32_t)mux) > data->src_count) {
		return -EINVAL;
	}

	new_rate = clock_get_rate(data->parents[(uint32_t)mux]);

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}

	ret = clock_children_notify_pre_change(clk_hw, curr_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	/* Apply source selection */
	data->src_sel = (uint32_t)mux;

	ret = clock_children_notify_post_change(clk_hw, curr_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int emul_clock_mux_notify(const struct clk *clk_hw, const struct clk *parent,
				 const struct clock_management_event *event)
{
	struct emul_clock_mux *data = clk_hw->hw_data;

	/*
	 * Read selector, and if index matches parent index we should notify
	 * children
	 */
	if (data->parents[data->src_sel] == parent) {
		return clock_notify_children(clk_hw, event);
	}

	/* Parent is not in use */
	return -ENOTCONN;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

static int emul_clock_mux_round_rate(const struct clk *clk_hw,
				     uint32_t req_rate)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	int cand_rate;
	int best_delta = INT32_MAX;
	int best_rate = 0;
	int target_rate = (int)req_rate;
	uint8_t idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < data->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(data->parents[idx], req_rate);
		if ((abs(cand_rate - target_rate) < best_delta) &&
		    (clock_children_check_rate(clk_hw, cand_rate) == 0)) {
			best_rate = cand_rate;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	return best_rate;
}

static int emul_clock_mux_set_rate(const struct clk *clk_hw,
				   uint32_t req_rate)
{
	struct emul_clock_mux *data = clk_hw->hw_data;
	int cand_rate, best_rate, ret, curr_rate;
	int best_delta = INT32_MAX;
	int target_rate = (int)req_rate;
	uint8_t idx = 0;
	uint8_t best_idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < data->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(data->parents[idx], req_rate);
		if ((abs(cand_rate - target_rate) < best_delta) &&
		    (clock_children_check_rate(clk_hw, cand_rate) == 0)) {
			best_idx = idx;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	/* Now set the clock rate for the best parent */
	best_rate = clock_set_rate(data->parents[best_idx], req_rate);
	if (best_rate < 0) {
		return best_rate;
	}

	curr_rate = clock_get_rate(clk_hw);

	ret = clock_children_notify_pre_change(clk_hw, curr_rate, best_rate);
	if (ret < 0) {
		return ret;
	}
	/* Set new parent selector */
	data->src_sel = best_idx;

	ret = clock_children_notify_post_change(clk_hw, curr_rate, best_rate);
	if (ret < 0) {
		return ret;
	}

	return best_rate;
}
#endif

const struct clock_management_driver_api emul_mux_api = {
	.get_rate = emul_clock_mux_get_rate,
	.configure = emul_clock_mux_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = emul_clock_mux_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
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
