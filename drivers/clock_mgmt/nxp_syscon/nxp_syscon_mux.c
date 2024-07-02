/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt/clock_driver.h>
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
	uint8_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t sel = ((*config->reg) & mux_mask) >> config->mask_offset;

	if (sel > config->src_count) {
		return -EIO;
	}

	return clock_get_rate(config->parents[sel]);
}

static int syscon_clock_mux_configure(const struct clk *clk_hw, const void *mux)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int ret;

	uint8_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t mux_val = FIELD_PREP(mux_mask, ((uint32_t)mux));

	if (((uint32_t)mux) > config->src_count) {
		return -EINVAL;
	}

	ret = clock_notify_children(clk_hw, clock_get_rate(config->parents[(uint32_t)mux]));
	if (ret < 0) {
		return ret;
	}

	(*config->reg) = ((*config->reg) & ~mux_mask) | mux_val;
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int syscon_clock_mux_notify(const struct clk *clk_hw, const struct clk *parent,
			    uint32_t parent_rate)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int ret;
	uint8_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t sel = ((*config->reg) & mux_mask) >> config->mask_offset;

	if (sel > config->src_count) {
		/* Notify children mux rate is 0 */
		clock_notify_children(clk_hw, 0);
		/* Selector has not been initialized */
		return -ENOTCONN;
	}

	/*
	 * Read mux reg, and if index matches parent index we should notify
	 * children
	 */
	if (config->parents[sel] == parent) {
		ret = clock_notify_children(clk_hw, parent_rate);
		if (ret < 0) {
			return ret;
		}
		if ((parent_rate == 0) && config->safe_mux) {
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

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int syscon_clock_mux_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int cand_rate;
	int best_delta = INT32_MAX;
	int best_rate = 0;
	int target_rate = rate;
	uint8_t idx = 0;

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < config->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(config->parents[idx], rate, clk_hw);
		if (abs(cand_rate - target_rate) < best_delta) {
			best_rate = cand_rate;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	return best_rate;
}

static int syscon_clock_mux_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int cand_rate, best_rate, ret;
	int best_delta = INT32_MAX;
	int target_rate = rate;
	uint32_t mux_val;
	uint8_t idx = 0;
	uint8_t best_idx = 0;
	uint8_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);

	/*
	 * Select a parent source based on the one able to
	 * provide the rate closest to what was requested by the
	 * caller
	 */
	while ((idx < config->src_count) && (best_delta > 0)) {
		cand_rate = clock_round_rate(config->parents[idx], rate, clk_hw);
		if (abs(cand_rate - target_rate) < best_delta) {
			best_idx = idx;
			best_delta = abs(cand_rate - target_rate);
		}
		idx++;
	}

	/* Now set the clock rate for the best parent */
	best_rate = clock_set_rate(config->parents[best_idx], rate, clk_hw);
	if (best_rate < 0) {
		return best_rate;
	}
	ret = clock_notify_children(clk_hw, best_rate);
	if (ret < 0) {
		return ret;
	}
	mux_val = FIELD_PREP(mux_mask, best_idx);
	if ((*config->reg & mux_mask) != mux_val) {
		/* Unlock the previous parent, so it can be reconfigured */
		clock_unlock(config->parents[(*config->reg) & mux_mask], clk_hw);
		(*config->reg) = ((*config->reg) & ~mux_mask) | mux_val;
	}

	return best_rate;
}
#endif

const struct clock_driver_api nxp_syscon_mux_api = {
	.get_rate = syscon_clock_mux_get_rate,
	.configure = syscon_clock_mux_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_clock_mux_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
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
