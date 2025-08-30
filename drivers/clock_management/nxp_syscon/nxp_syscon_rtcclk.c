/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>

#define DT_DRV_COMPAT nxp_syscon_rtcclk

struct syscon_rtcclk_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	uint16_t add_factor;
	uint8_t mask_offset;
	uint8_t mask_width;
	volatile uint32_t *reg;
};


static int syscon_clock_rtcclk_recalc_rate(const struct clk *clk_hw,
					   uint32_t parent_rate,
					   uint32_t *output_rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_factor = (*config->reg & div_mask) + config->add_factor;

	/* Calculate divided clock */
	*output_rate = parent_rate / div_factor;
	return 0;
}

static int syscon_clock_rtcclk_configure(const struct clk *clk_hw, const void *div_cfg)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t div_val = ((uint32_t)div_cfg) - config->add_factor;
	uint32_t div_raw = FIELD_PREP(div_mask, div_val);

	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int syscon_clock_rtcclk_recalc_configure(const struct clk *clk_hw,
						const void *div_cfg,
						uint32_t parent_rate,
						uint32_t *output_rate)
{
	*output_rate = parent_rate / ((uint32_t)div_cfg);

	return 0;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int syscon_clock_rtcclk_round_rate(const struct clk *clk_hw,
					  uint32_t rate_req,
					  uint32_t parent_rate,
					  uint32_t *output_rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int ret;
	uint32_t div_raw, div_factor;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t parent_req = rate_req * config->add_factor;


	/*
	 * Request a parent rate at the lower end of the frequency range
	 * this RTC divider can handle
	 */
	ret = clock_management_round_rate(GET_CLK_PARENT(clk_hw), parent_req,
					  &parent_rate);
	if (ret < 0) {
		return ret;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = fin / fout - add_factor
	 */
	div_raw = (parent_rate / rate_req) - config->add_factor;
	div_factor = (div_raw & div_mask) + config->add_factor;

	*output_rate = parent_rate / div_factor;
	return 0;
}

static int syscon_clock_rtcclk_set_rate(const struct clk *clk_hw,
					uint32_t rate_req,
					uint32_t parent_rate,
					uint32_t *output_rate)
{
	const struct syscon_rtcclk_config *config = clk_hw->hw_data;
	int ret;
	uint32_t div_raw, div_factor;
	uint8_t div_mask = GENMASK((config->mask_width +
				   config->mask_offset - 1),
				   config->mask_offset);
	uint32_t parent_req = rate_req * config->add_factor;


	/*
	 * Request a parent rate at the lower end of the frequency range
	 * this RTC divider can handle
	 */
	ret = clock_management_round_rate(GET_CLK_PARENT(clk_hw), parent_req,
					  &parent_rate);
	if (ret < 0) {
		return ret;
	}
	/*
	 * Formula for the target RTC clock div setting is given
	 * by the following:
	 * reg_val = fin / fout - add_factor
	 */
	div_raw = (parent_rate / rate_req) - config->add_factor;
	div_factor = (div_raw & div_mask) + config->add_factor;

	(*config->reg) = ((*config->reg) & ~div_mask) | div_raw;
	*output_rate = parent_rate / div_factor;

	return 0;
}
#endif

const struct clock_management_standard_api nxp_syscon_rtcclk_api = {
	.recalc_rate = syscon_clock_rtcclk_recalc_rate,
	.shared.configure = syscon_clock_rtcclk_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = syscon_clock_rtcclk_recalc_configure,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_clock_rtcclk_round_rate,
	.set_rate = syscon_clock_rtcclk_set_rate,
#endif
};

#define NXP_RTCCLK_DEFINE(inst)                                                \
	const struct syscon_rtcclk_config nxp_syscon_rtcclk_##inst = {         \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
		.mask_width = DT_INST_REG_SIZE(inst),                          \
		.mask_offset = DT_INST_PROP(inst, offset),                     \
		.add_factor = DT_INST_PROP(inst, add_factor),                  \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &nxp_syscon_rtcclk_##inst,                        \
			     &nxp_syscon_rtcclk_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_RTCCLK_DEFINE)
