/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#include <wrap_max32_sys.h>

#define DT_DRV_COMPAT adi_max32_clock_div

/* Valid values are from 0 to 7 for the clock divider */
#define MAX32_CLOCK_DIVIDER_RANGE (7U)

struct max32_clock_div_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	volatile uint32_t *reg;
	uint8_t mask_width;
	uint8_t mask_offset;
};

static int max32_clock_div_configure(const struct clk *clk_hw, const void *data)
{
	const struct max32_clock_div_config *config = clk_hw->hw_data;
	const uint32_t div_setting = (uint32_t)(uintptr_t)data;

	if (config->reg == NULL) {
		return -EINVAL;
	}

	if (div_setting > MAX32_CLOCK_DIVIDER_RANGE) {
		return -EINVAL;
	}

	Wrap_MXC_SYS_SetClockDiv(div_setting << MXC_F_GCR_CLKCTRL_SYSCLK_DIV_POS);

	return 0;
}

static clock_freq_t max32_clock_div_recalc_rate(const struct clk *clk_hw, clock_freq_t parent_rate)
{
	const struct max32_clock_div_config *config = clk_hw->hw_data;
	const uint32_t div_setting = (*config->reg >> config->mask_offset) & GENMASK((config->mask_width - 1), 0);

	if (config->reg == NULL) {
		return -EINVAL;
	}

	return (parent_rate >> div_setting);
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t max32_clock_div_configure_recalc(const struct clk *clk_hw, const void *data, clock_freq_t parent_rate)
{
	ARG_UNUSED(clk_hw);
	const uint32_t div_setting = (uint32_t)(uintptr_t)data;

	if (div_setting > MAX32_CLOCK_DIVIDER_RANGE) {
		return -EINVAL;
	}

	return (parent_rate >> div_setting);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t max32_clock_div_best_rate(const struct clk *clk_hw, clock_freq_t rate_req, clock_freq_t parent_rate, bool commit)
{
	int ret;
	const struct max32_clock_div_config *config = clk_hw->hw_data;

	if (config->reg == NULL || rate_req == 0) {
		return -EINVAL;
	}

	const uint32_t div_setting = MIN(MAX(LOG2(parent_rate / rate_req), 0), MAX32_CLOCK_DIVIDER_RANGE);
	const clock_freq_t best_rate = (parent_rate >> div_setting);

	if (commit) {
		ret = max32_clock_div_configure(clk_hw, (const void *)(uintptr_t)div_setting);
		if (ret < 0) {
			return ret;
		}
	}

	return best_rate;
}
#endif

const struct clock_management_standard_api max32_clock_div_api = {
	.shared.configure = max32_clock_div_configure,
	.recalc_rate = max32_clock_div_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = max32_clock_div_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.best_rate = max32_clock_div_best_rate,
#endif
};

#define ADI_MAX32_CLOCK_DIV_DEFINE(inst) \
	static const struct max32_clock_div_config max32_clock_div_config_##inst = { \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_PHANDLE(DT_DRV_INST(inst), input_source))) \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst), \
		.mask_width = (uint8_t)DT_INST_REG_SIZE(inst), \
		.mask_offset = (uint8_t)DT_INST_PROP(inst, offset), \
	}; \
	CLOCK_DT_INST_DEFINE(inst, &max32_clock_div_config_##inst, &max32_clock_div_api);

DT_INST_FOREACH_STATUS_OKAY(ADI_MAX32_CLOCK_DIV_DEFINE)
