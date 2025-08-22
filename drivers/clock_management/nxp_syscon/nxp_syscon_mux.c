/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>
#include <stdlib.h>

#include "nxp_syscon_internal.h"

#define DT_DRV_COMPAT nxp_syscon_clock_mux

struct syscon_clock_mux_config {
	MUX_CLK_SUBSYS_DATA_DEFINE
	uint8_t mask_width;
	uint8_t mask_offset;
	uint8_t safe_mux;
	volatile uint32_t *reg;
};

static int syscon_clock_mux_get_parent(const struct clk *clk_hw)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	uint32_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint8_t sel = ((*config->reg) & mux_mask) >> config->mask_offset;

	if (sel >= config->parent_cnt) {
		return -ENOTCONN;
	}

	return sel;
}

static int syscon_clock_mux_configure(const struct clk *clk_hw, const void *mux)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;

	uint32_t mux_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t mux_val = FIELD_PREP(mux_mask, ((uint32_t)mux));

	if (((uint32_t)mux) > config->parent_cnt) {
		return -EINVAL;
	}

	(*config->reg) = ((*config->reg) & ~mux_mask) | mux_val;

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_mux_configure_recalc(const struct clk *clk_hw,
					     const void *mux)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;

	if (((uint32_t)mux) > config->parent_cnt) {
		return -EINVAL;
	}

	return (int)(uintptr_t)mux;
}

static int syscon_clock_mux_validate_parent(const struct clk *clk_hw,
					    clock_freq_t parent_freq, uint8_t new_idx)
{
	const struct syscon_clock_mux_config *config = clk_hw->hw_data;
	int ret;

	if (new_idx >= config->parent_cnt) {
		return -EINVAL;
	}

	/*
	 * Some syscon multiplexers are "safe", meaning they will not switch
	 * sources unless both the current and new source are valid.
	 * To prevent such a switch, don't permit moving to a new
	 * source if the frequency is 0.
	 *
	 * Note that some parent drivers (such as PLLs) will gate during
	 * reconfiguration, and receive an error from this function. To work
	 * around this we use a specific return code, so the driver knows the
	 * syscon mux is the driver preventing reconfiguration. The parent
	 * driver can then ignore the error if it knowns it will restore
	 * a valid frequency before returning.
	 *
	 * This allows parent drivers to distinguish between the case where the
	 * mux is preventing gating, and the case where another consumer cannot
	 * accept the parent gating.
	 */
	if (config->safe_mux && (parent_freq == 0)) {
		/* Check with child clocks to make sure they will accept gating */
		ret = clock_children_check_rate(clk_hw, 0);
		if (ret < 0) {
			/* Some child can't accept gating */
			return ret;
		}
		/* Only the mux can't gate- indicate this */
		return NXP_SYSCON_MUX_ERR_SAFEGATE;
	}

	return 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_mux_set_parent(const struct clk *clk_hw,
				      uint8_t new_idx)
{
	return syscon_clock_mux_configure(clk_hw, (const void *)(uintptr_t)new_idx);
}
#endif

const struct clock_management_mux_api nxp_syscon_mux_api = {
	.get_parent = syscon_clock_mux_get_parent,
	.shared.configure = syscon_clock_mux_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.mux_configure_recalc = syscon_clock_mux_configure_recalc,
	.mux_validate_parent = syscon_clock_mux_validate_parent,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.set_parent = syscon_clock_mux_set_parent,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx)                                      \
	CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define NXP_SYSCON_CLOCK_DEFINE(inst)                                          \
	const struct clk *const nxp_syscon_mux_##inst##_parents[] = {          \
		DT_INST_FOREACH_PROP_ELEM(inst, input_sources,                 \
						GET_MUX_INPUT)                 \
	};                                                                     \
	const struct syscon_clock_mux_config nxp_syscon_mux_##inst = {         \
		MUX_CLK_SUBSYS_DATA_INIT(nxp_syscon_mux_##inst##_parents,      \
				 DT_INST_PROP_LEN(inst, input_sources))        \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.mask_width = (uint8_t)DT_INST_REG_SIZE(inst),                 \
		.mask_offset = (uint8_t)DT_INST_PROP(inst, offset),            \
		.safe_mux = DT_INST_PROP(inst, safe_mux),                      \
	};                                                                     \
	                                                                       \
	MUX_CLOCK_DT_INST_DEFINE(inst,                                         \
				 &nxp_syscon_mux_##inst,                       \
				 &nxp_syscon_mux_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SYSCON_CLOCK_DEFINE)
