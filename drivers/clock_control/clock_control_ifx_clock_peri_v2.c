/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <cy_sysclk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_clock_peri_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_clk_peri

#define IFX_FRACTION_DIVIDER_VALUE 32
#define IFX_PERI_CLK_MAX_DIVIDER   (IFX_PERI_DIV_24_5 + 1)

#define IFX_PERI_DIV_8    0
#define IFX_PERI_DIV_16   1
#define IFX_PERI_DIV_16_5 2
#define IFX_PERI_DIV_24_5 3

struct ifx_clock_peri_config {
	uint32_t peri_clk_inst;
	uint8_t max_dividers[IFX_PERI_CLK_MAX_DIVIDER];
	uint8_t src_clk_id;
};

struct ifx_clock_peri_data {
	uint32_t src_clk_freq;
	struct k_mutex lock;
};

static int clock_control_ifx_clock_peri_get_rate(const struct device *dev,
						 clock_control_subsys_t sys, uint32_t *rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clock_peri_data *data = dev->data;

	if ((rate == NULL) || (clk == NULL)) {
		return -EINVAL;
	}

	if (clk->divider_inst >= cfg->max_dividers[clk->divider_type]) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	*rate = Cy_SysClk_PeriPclkGetFrequency(cfg->peri_clk_inst, clk->divider_type,
					       clk->divider_inst);

	k_mutex_unlock(&data->lock);

	return 0;
}

static inline int check_divider_range(uint32_t div_value, uint8_t div_type)
{
	int max_div_value;

	switch (div_type) {
	case IFX_PERI_DIV_8:
		max_div_value = (1 << 8);
		break;
	case IFX_PERI_DIV_16:
	case IFX_PERI_DIV_16_5:
		max_div_value = (1 << 16);
		break;
	case IFX_PERI_DIV_24_5:
		max_div_value = (1 << 24);
		break;
	default:
		return -1;
	}

	if (div_value > max_div_value) {
		return -1;
	}

	return 0;
}

static int clock_control_ifx_clock_peri_set_rate(const struct device *dev,
						 clock_control_subsys_t sys,
						 clock_control_subsys_rate_t rate)
{
	struct ifx_clk_peri *clk = (struct ifx_clk_peri *)sys;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clock_peri_data *data = dev->data;
	uint32_t *clk_rate = (uint32_t *)rate;
	uint32_t clock_id;
	uint32_t int_div;
	int ret = 0;

	if ((rate == NULL) || (sys == NULL)) {
		return -EINVAL;
	}

	if ((data->src_clk_freq < *clk_rate) || (*clk_rate == 0)) {
		return -EINVAL;
	}

	if (clk->divider_inst >= cfg->max_dividers[clk->divider_type]) {
		return -EINVAL;
	}

	int_div = (data->src_clk_freq / *clk_rate);

	if (check_divider_range(int_div, clk->divider_type)) {
		LOG_ERR("Invalid Integer Divider value with Divider type");
		return -ERANGE;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	clock_id = cfg->peri_clk_inst | clk->rootclk_id;
	if (Cy_SysClk_PeriPclkDisableDivider(clock_id, clk->divider_type, clk->divider_inst)) {
		ret = -EIO;
		goto out;
	}

	switch (clk->divider_type) {
	case IFX_PERI_DIV_8:
	case IFX_PERI_DIV_16:
		if (Cy_SysClk_PeriPclkSetDivider(clock_id, clk->divider_type, clk->divider_inst,
						 int_div - 1U)) {
			ret = -EIO;
			goto out;
		}

		break;
	case IFX_PERI_DIV_16_5:
	case IFX_PERI_DIV_24_5: {
		uint8_t frac_div = (((uint64_t)(data->src_clk_freq - int_div * (*clk_rate)) *
				    IFX_FRACTION_DIVIDER_VALUE) / *clk_rate);

		if (frac_div > IFX_FRACTION_DIVIDER_VALUE) {
			LOG_ERR("Invalid Fractional Divider value with Divider type");
			ret = -ERANGE;
			goto out;
		}

		if (Cy_SysClk_PeriPclkSetFracDivider(clock_id, clk->divider_type,
						     clk->divider_inst, int_div - 1U,
						     (uint32_t)frac_div)) {
			ret = -EIO;
			goto out;
		}

		break;
	}
	default:
		ret = -EINVAL;
		goto out;
	}

	if (Cy_SysClk_PeriPclkEnableDivider(clock_id, clk->divider_type, clk->divider_inst)) {
		ret = -EIO;
		goto out;
	}

	if (Cy_SysClk_PeriPclkAssignDivider(clock_id, clk->divider_type, clk->divider_inst)) {
		ret = -EIO;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int clock_control_ifx_clock_peri_init(const struct device *dev)
{
	struct ifx_clock_peri_data *data = dev->data;
	const struct ifx_clock_peri_config *cfg = dev->config;
	struct ifx_clk clk = {0};

	clk.clk = IFX_CLK_HF;
	clk.clk_id = cfg->src_clk_id;

	k_mutex_init(&data->lock);

	return clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(clocks)),
				      (clock_control_subsys_t *)&clk, &data->src_clk_freq);
}

static DEVICE_API(clock_control, ifx_clock_peri_control_api) = {
	.get_rate = clock_control_ifx_clock_peri_get_rate,
	.set_rate = clock_control_ifx_clock_peri_set_rate,
};

#define FILL_DIVIDER_INST(inst)                                                                    \
	.max_dividers[IFX_PERI_DIV_8] = DT_INST_PROP_OR(inst, ifx_8_bit_max_divider, 0),           \
	.max_dividers[IFX_PERI_DIV_16] = DT_INST_PROP_OR(inst, ifx_16_bit_max_divider, 0),         \
	.max_dividers[IFX_PERI_DIV_16_5] = DT_INST_PROP_OR(inst, ifx_16_5_bit_max_divider, 0),     \
	.max_dividers[IFX_PERI_DIV_24_5] = DT_INST_PROP_OR(inst, ifx_24_5_bit_max_divider, 0),

#define IFX_CLOCK_PERI_INIT(inst)                                                                  \
	static const struct ifx_clock_peri_config clk_cfg_##inst = {                               \
		.src_clk_id = DT_REG_ADDR(DT_INST_PHANDLE(inst, clocks)),                          \
		.peri_clk_inst = DT_INST_PROP_OR(inst, ifx_peri_clk_inst, 0),                      \
		FILL_DIVIDER_INST(inst)};                                                          \
                                                                                                   \
	static struct ifx_clock_peri_data clk_data_##inst;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, clock_control_ifx_clock_peri_init, NULL, &clk_data_##inst,     \
			      &clk_cfg_##inst, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,   \
			      &ifx_clock_peri_control_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CLOCK_PERI_INIT)
