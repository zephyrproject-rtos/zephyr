/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT vnd_emul_clock_div

struct emul_clock_div {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	uint8_t div_max;
	uint8_t div_val;
};

static int emul_clock_div_configure(const struct clk *clk_hw, const void *div_cfg)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	uint32_t div_val = (uint32_t)(uintptr_t)div_cfg;

	/* Apply div selection */
	data->div_val = div_val - 1;
	return 0;
}

static clock_freq_t emul_clock_div_recalc_rate(const struct clk *clk_hw,
				      clock_freq_t parent_rate)
{
	struct emul_clock_div *data = clk_hw->hw_data;

	return (parent_rate / (data->div_val + 1));
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t emul_clock_div_configure_recalc(const struct clk *clk_hw,
					   const void *div_cfg,
					   clock_freq_t parent_rate)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	uint32_t div_val = (uint32_t)(uintptr_t)div_cfg;

	if ((div_val < 1) || (div_val > (data->div_max + 1))) {
		return -EINVAL;
	}

	return parent_rate / div_val;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t emul_clock_div_round_rate(const struct clk *clk_hw,
				     clock_freq_t req_rate,
				     clock_freq_t parent_rate)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	int div_val = CLAMP((parent_rate / req_rate), 1, (data->div_max + 1));
	clock_freq_t output_rate = parent_rate / div_val;

	/* Raise div value until we are in range */
	while (output_rate > req_rate) {
		div_val++;
		output_rate = parent_rate / div_val;
	}

	if (output_rate > req_rate) {
		return -ENOENT;
	}

	return output_rate;
}

static clock_freq_t emul_clock_div_set_rate(const struct clk *clk_hw,
				   clock_freq_t req_rate,
				   clock_freq_t parent_rate)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	int div_val = CLAMP((parent_rate / req_rate), 1, (data->div_max + 1));
	clock_freq_t output_rate = parent_rate / div_val;

	/* Raise div value until we are in range */
	while (output_rate > req_rate) {
		div_val++;
		output_rate = parent_rate / div_val;
	}

	if (output_rate > req_rate) {
		return -ENOENT;
	}

	data->div_val = div_val - 1;

	return output_rate;
}
#endif

const struct clock_management_standard_api emul_div_api = {
	.recalc_rate = emul_clock_div_recalc_rate,
	.shared.configure = emul_clock_div_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = emul_clock_div_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = emul_clock_div_round_rate,
	.set_rate = emul_clock_div_set_rate,
#endif
};

#define EMUL_CLOCK_DEFINE(inst)                                                \
	struct emul_clock_div emul_clock_div_##inst = {                        \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.div_max = DT_INST_PROP(inst, max_div) - 1,                    \
		.div_val = 0,                                                  \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &emul_clock_div_##inst,                           \
			     &emul_div_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_CLOCK_DEFINE)
