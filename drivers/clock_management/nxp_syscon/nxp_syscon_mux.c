/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <stdlib.h>

#include "nxp_syscon_internal.h"

#define DT_DRV_COMPAT nxp_syscon_clock_mux

struct syscon_clock_mux_config {
	uint8_t mask_width;
	uint8_t mask_offset;
	uint8_t src_count;
	uint8_t safe_mux;
	volatile uint32_t *reg;
	const struct clk *parents[];
};

static int syscon_clock_mux_get_rate(const struct clk *clk_hw)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	uint32_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t sel = ((*config->reg) & mux_mask) >> config->mask_offset;

	if (sel >= config->src_count) {
		return -EIO;
	}

	return clock_get_rate(config->parents[sel]);
}

static int syscon_clock_mux_configure(const struct clk *clk_hw, const void *mux)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int ret;

	uint32_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t mux_val = FIELD_PREP(mux_mask, ((uint32_t)mux));
	uint32_t cur_rate = clock_get_rate(clk_hw);
	uint32_t new_rate;

	if (((uint32_t)mux) > config->src_count) {
		return -EINVAL;
	}

	new_rate = clock_get_rate(config->parents[(uint32_t)mux]);

	ret = clock_children_check_rate(clk_hw, new_rate);
	if (ret < 0) {
		return ret;
	}

	ret = clock_children_notify_pre_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}

	(*config->reg) = ((*config->reg) & ~mux_mask) | mux_val;

	ret = clock_children_notify_post_change(clk_hw, cur_rate, new_rate);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_mux_notify(const struct clk *clk_hw, const struct clk *parent,
				   const struct clock_management_event *event)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int ret;
	uint8_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t sel = ((*config->reg) & mux_mask) >> config->mask_offset;
	struct clock_management_event notify_event;

	notify_event.type = event->type;

	if (sel >= config->src_count) {
		notify_event.old_rate = 0;
		notify_event.new_rate = 0;
		/* Notify children mux rate is 0 */
		clock_notify_children(clk_hw, &notify_event);
		/* Selector has not been initialized */
		return -ENOTCONN;
	}

	/*
	 * Read mux reg, and if index matches parent index we should notify
	 * children
	 */
	if (config->parents[sel] == parent) {
		notify_event.old_rate = event->old_rate;
		notify_event.new_rate = event->new_rate;
		ret = clock_notify_children(clk_hw, &notify_event);
		if (ret < 0) {
			return ret;
		}
		if ((event->new_rate == 0) && config->safe_mux) {
			/* These muxes are "fail-safe",
			 * which means they refuse to switch clock outputs
			 * if the one they are using is gated.
			 */
			ret = NXP_SYSCON_MUX_ERR_SAFEGATE;
		}
		return ret;
	}

	/* Parent is not in use */
	return -ENOTCONN;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_mux_round_rate(const struct clk *clk_hw,
				       uint32_t rate_req)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int cand_rate;
	int best_delta = INT32_MAX;
	int best_rate = 0;
	int target_rate = (int)rate_req;
	uint8_t idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < config->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(config->parents[idx], rate_req);
		if ((abs(cand_rate - target_rate) < best_delta) &&
		    (clock_children_check_rate(clk_hw, cand_rate) >= 0)) {
			best_rate = cand_rate;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	return best_rate;
}

static int syscon_clock_mux_set_rate(const struct clk *clk_hw,
				     uint32_t rate_req)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int cand_rate, best_rate, ret;
	int best_delta = INT32_MAX;
	uint32_t mux_val, cur_rate;
	int target_rate = (int)rate_req;
	uint32_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t idx = 0;
	uint8_t best_idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < config->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(config->parents[idx], rate_req);
		if ((abs(cand_rate - target_rate) < best_delta) &&
		    (clock_children_check_rate(clk_hw, cand_rate) >= 0)) {
			best_idx = idx;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	/* Now set the clock rate for the best parent */
	best_rate = clock_set_rate(config->parents[best_idx], rate_req);
	if (best_rate < 0) {
		return best_rate;
	}
	cur_rate = clock_get_rate(clk_hw);
	ret = clock_children_notify_pre_change(clk_hw, cur_rate, best_rate);
	if (ret < 0) {
		return ret;
	}
	mux_val = FIELD_PREP(mux_mask, best_idx);
	if ((*config->reg & mux_mask) != mux_val) {
		(*config->reg) = ((*config->reg) & ~mux_mask) | mux_val;
	}

	ret = clock_children_notify_post_change(clk_hw, cur_rate, best_rate);
	if (ret < 0) {
		return ret;
	}

	return best_rate;
}
#endif

const struct clock_management_driver_api nxp_syscon_mux_api = {
	.get_rate = syscon_clock_mux_get_rate,
	.configure = syscon_clock_mux_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = syscon_clock_mux_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_mux_round_rate,
	.set_rate = syscon_clock_mux_set_rate,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx)                                      \
	CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct syscon_clock_mux_config nxp_syscon_mux_##inst = {         \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.mask_width = (uint8_t)DT_INST_REG_SIZE(inst),                 \
		.mask_offset = (uint8_t)DT_INST_PROP(inst, offset),            \
		.src_count = DT_INST_PROP_LEN(inst, input_sources),            \
		.safe_mux = DT_INST_PROP(inst, safe_mux),                      \
		.parents = {                                                   \
			DT_INST_FOREACH_PROP_ELEM(inst, input_sources,         \
						GET_MUX_INPUT)                 \
		},                                                             \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_mux_##inst,                           \
			     &nxp_syscon_mux_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
