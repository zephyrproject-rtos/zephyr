/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include "emul_clock_internal.h"
#include "emul_clock_drivers.h"

#define DT_DRV_COMPAT vnd_emul_clock_virt

#define VIRT_DIV_MAX  255
#define VIRT_MULT_MAX 255

/* Multiplexer driver. Reuses the emul_clock_mux implementation */

static const struct clock_management_mux_api emul_mux_api = {
	.get_parent = emul_clock_mux_get_parent,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.mux_configure_recalc = emul_clock_mux_configure_recalc,
	.mux_validate_parent = emul_clock_mux_validate_parent,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.set_parent = emul_clock_mux_set_parent,
#endif
};

#define GET_MUX_INPUT(node_id, prop, idx) CLOCK_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define EMUL_CLOCK_VIRT_MUX_INIT(inst)                                                             \
	static const struct clk *const emul_clock_mux_parent_##inst[] = {                          \
		DT_INST_FOREACH_PROP_ELEM(inst, inputs, GET_MUX_INPUT)};                           \
	static struct emul_clock_mux emul_clock_mux_data_##inst = {                                \
		MUX_CLK_SUBSYS_DATA_INIT(emul_clock_mux_parent_##inst,                             \
					 DT_INST_PROP_LEN(inst, inputs))                           \
			.src_count = DT_INST_PROP_LEN(inst, inputs),                               \
		.src_sel = 0,                                                                      \
	};                                                                                         \
                                                                                                   \
	MUX_CLOCK_DT_DEFINE(DT_INST_CHILD(inst, virt_mux), &emul_clock_mux_data_##inst,            \
			    &emul_mux_api, emul_mult_clock_##inst)

/* Divider implementation. Reuses the emul_clock_div implementation */

static const struct clock_management_standard_api emul_div_api = {
	.recalc_rate = emul_clock_div_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = emul_clock_div_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.best_rate = emul_clock_div_best_rate,
#endif
};

#define EMUL_CLOCK_VIRT_DIV_INIT(inst)                                                             \
	static struct emul_clock_div emul_clock_div_data_##inst = {                                \
		STANDARD_CLK_SUBSYS_DATA_INIT(&CLOCK_NAME_GET(emul_mult_clock_##inst)).div_max =   \
			VIRT_DIV_MAX,                                                              \
		.div_val = 1,                                                                      \
	};                                                                                         \
	CLOCK_DT_DEFINE(DT_INST_CHILD(inst, virt_div), &emul_clock_div_data_##inst, &emul_div_api)

/* Multiplier implementation. This clock is software only (no DT node) */

struct emul_clock_mult {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	uint32_t mult_val;
	uint32_t mult_max;
};

static int emul_clock_mult_configure(const struct clk *clk_hw, const void *mult_cfg)
{
	struct emul_clock_mult *mult = clk_hw->hw_data;
	uint32_t mult_val = (uint32_t)(uintptr_t)mult_cfg;

	if (mult_val > VIRT_MULT_MAX) {
		return -EINVAL;
	}

	mult->mult_val = mult_val;
	return 0;
}

static clock_freq_t emul_clock_mult_recalc_rate(const struct clk *clk_hw, clock_freq_t parent_rate)
{
	struct emul_clock_mult *mult = clk_hw->hw_data;

	return parent_rate * mult->mult_val;
}

#ifdef CONFIG_CLOCK_MANAGEMENT_RUNTIME
static clock_freq_t emul_clock_mult_configure_recalc(const struct clk *clk_hw, const void *mult_cfg,
						     clock_freq_t parent_rate)
{
	uint32_t mult_val = (uint32_t)(uintptr_t)mult_cfg;

	ARG_UNUSED(clk_hw);
	if (mult_val > VIRT_MULT_MAX) {
		return -EINVAL;
	}

	return parent_rate * mult_val;
}
#endif

#ifdef CONFIG_CLOCK_MANAGEMENT_SET_RATE
static int emul_clock_mult_best_rate(const struct clk *clk_hw, clock_freq_t req_rate,
				     clock_freq_t parent_rate, bool commit)
{
	struct emul_clock_mult *mult = clk_hw->hw_data;
	int mult_val = CLAMP(req_rate / parent_rate, 1, VIRT_MULT_MAX);

	if (commit) {
		mult->mult_val = mult_val;
	}

	return parent_rate * mult_val;
}
#endif

static const struct clock_management_standard_api emul_mult_api = {
	.recalc_rate = emul_clock_mult_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = emul_clock_mult_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.best_rate = emul_clock_mult_best_rate,
#endif
};

#define EMUL_CLOCK_VIRT_MULT_INIT(inst)                                                            \
	static struct emul_clock_mult emul_clock_mult_data_##inst = {                              \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_CHILD(inst, virt_mux)))         \
			.mult_max = VIRT_MULT_MAX,                                                 \
		.mult_val = 1,                                                                     \
	};                                                                                         \
	CLOCK_DEFINE(emul_mult_clock_##inst, &emul_clock_mult_data_##inst, &emul_mult_api)

/* Virtual Clock Implementation. This clock doesn't affect rate, just
 * configures the virtual clocks in this file
 */

struct emul_clock_virt {
	const struct clk *div;
	const struct clk *mult;
	const struct clk *mux;
};

static int emul_clock_virt_configure(const struct clk *clk_hw, const void *virt_cfg)
{
	struct emul_clock_virt *data = clk_hw->hw_data;
	uint32_t virt_cfg_val = (uint32_t)(uintptr_t)virt_cfg;
	uint8_t mux_val = (virt_cfg_val & VIRT_MUX_MASK) >> VIRT_MUX_SHIFT;
	uint8_t div_val = (virt_cfg_val & VIRT_DIV_MASK) >> VIRT_DIV_SHIFT;
	uint8_t mult_val = (virt_cfg_val & VIRT_MULT_MASK) >> VIRT_MULT_SHIFT;
	int ret;

	/* Configure the virtual clocks */
	ret = emul_clock_mux_configure(data->mux, (void *)(uintptr_t)mux_val);
	if (ret) {
		return ret;
	}
	ret = emul_clock_div_configure(data->div, (void *)(uintptr_t)div_val);
	if (ret) {
		return ret;
	}
	ret = emul_clock_mult_configure(data->mult, (void *)(uintptr_t)mult_val);
	if (ret) {
		return ret;
	}
	return 0;
}

static bool emul_clock_virt_query(const struct clk *clk_hw, const void *virt_cfg,
				  const struct clk *tgt, void **data)
{
	struct emul_clock_virt *hw_data = clk_hw->hw_data;
	uint32_t virt_cfg_val = (uint32_t)(uintptr_t)virt_cfg;
	uint8_t mux_val = (virt_cfg_val & VIRT_MUX_MASK) >> VIRT_MUX_SHIFT;
	uint8_t div_val = (virt_cfg_val & VIRT_DIV_MASK) >> VIRT_DIV_SHIFT;
	uint8_t mult_val = (virt_cfg_val & VIRT_MULT_MASK) >> VIRT_MULT_SHIFT;

	if (tgt == hw_data->mux) {
		*data = (void *)(uintptr_t)mux_val;
		return true;
	}
	if (tgt == hw_data->div) {
		*data = (void *)(uintptr_t)div_val;
		return true;
	}
	if (tgt == hw_data->mult) {
		*data = (void *)(uintptr_t)mult_val;
		return true;
	}
	return false;
}

static struct clock_management_virtual_api emul_clock_virt_api = {
	.shared.configure = emul_clock_virt_configure,
	.query = emul_clock_virt_query,
};

#define EMUL_CLOCK_VIRT_DEFINE(inst)                                                               \
	extern const struct clk CLOCK_NAME_GET(emul_mult_clock_##inst);                            \
	EMUL_CLOCK_VIRT_MUX_INIT(inst);                                                            \
	EMUL_CLOCK_VIRT_DIV_INIT(inst);                                                            \
	EMUL_CLOCK_VIRT_MULT_INIT(inst);                                                           \
	static struct emul_clock_virt emul_clock_virt_data_##inst = {                              \
		.div = CLOCK_DT_GET(DT_INST_CHILD(inst, virt_div)),                                \
		.mult = &CLOCK_NAME_GET(emul_mult_clock_##inst),                                   \
		.mux = CLOCK_DT_GET(DT_INST_CHILD(inst, virt_mux)),                                \
	};                                                                                         \
	VIRTUAL_CLOCK_DT_INST_DEFINE(inst, &emul_clock_virt_data_##inst, &emul_clock_virt_api,     \
				     emul_mult_clock_##inst)

DT_INST_FOREACH_STATUS_OKAY(EMUL_CLOCK_VIRT_DEFINE);
