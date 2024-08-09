/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_pll

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_PLL_A_Type.h>
#include <si32_device.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pll);

struct clock_control_si32_pll_config {
	SI32_PLL_A_Type *pll;
};

struct clock_control_si32_pll_data {
	uint32_t freq;
};

static int clock_control_si32_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_si32_pll_config *config = dev->config;
	struct clock_control_si32_pll_data *data = dev->data;

	if (data->freq == 0) {
		return -ENOTSUP;
	}

	uint32_t dco_range;

	if (data->freq > 80000000) {
		dco_range = 4;
	} else if (data->freq > 76500000) {
		dco_range = 4;
	} else if (data->freq > 62000000) {
		dco_range = 3;
	} else if (data->freq > 49500000) {
		dco_range = 2;
	} else if (data->freq > 35000000) {
		dco_range = 1;
	} else if (data->freq > 23000000) {
		dco_range = 0;
	} else {
		return -ENOTSUP;
	}

	/* lposc0div */
	const uint32_t source_clock_freq = 2500000;
	const uint32_t div_m = 100;
	const uint32_t div_n = (data->freq / source_clock_freq) * (div_m + 1) - 1;

	if (div_n < 32 || div_n > 4095) {
		return -ENOTSUP;
	}

	/* Setup PLL to lock to requested frequency */
	SI32_PLL_A_initialize(config->pll, 0x00, 0x00, 0x00, 0x000FFF0);
	SI32_PLL_A_set_numerator(config->pll, div_n);
	SI32_PLL_A_set_denominator(config->pll, div_m);
	/* TODO: support other clock sources */
	SI32_PLL_A_select_reference_clock_source_lp0oscdiv(config->pll);

	/* Wait for lock */
	SI32_PLL_A_select_disable_dco_output(config->pll);
	SI32_PLL_A_set_frequency_adjuster_value(config->pll, 0xFFF);
	SI32_PLL_A_set_output_frequency_range(config->pll, dco_range);

	/* Lock and block for result */
	SI32_PLL_A_select_dco_frequency_lock_mode(config->pll);
	while (!(SI32_PLL_A_is_locked(config->pll) ||
		 SI32_PLL_A_is_saturation_low_interrupt_pending(config->pll) ||
		 SI32_PLL_A_is_saturation_high_interrupt_pending(config->pll)))
		;

	return 0;
}

static int clock_control_si32_pll_off(const struct device *dev, clock_control_subsys_t sys)
{

	return -ENOTSUP;
}

static int clock_control_si32_pll_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	struct clock_control_si32_pll_data *data = dev->data;

	*rate = data->freq;

	return 0;
}

static int clock_control_si32_pll_set_rate(const struct device *dev, clock_control_subsys_t sys,
					   clock_control_subsys_rate_t rate_)
{
	struct clock_control_si32_pll_data *data = dev->data;
	const uint32_t *rate = rate_;

	data->freq = *rate;

	return 0;
}

static struct clock_control_driver_api clock_control_si32_pll_api = {
	.on = clock_control_si32_pll_on,
	.off = clock_control_si32_pll_off,
	.get_rate = clock_control_si32_pll_get_rate,
	.set_rate = clock_control_si32_pll_set_rate,
};

static int clock_control_si32_pll_init(const struct device *dev)
{
	SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0, SI32_CLKCTRL_A_APBCLKG0_PLL0);

	return 0;
}

static const struct clock_control_si32_pll_config config = {
	.pll = (SI32_PLL_A_Type *)DT_REG_ADDR(DT_NODELABEL(pll0)),
};

static struct clock_control_si32_pll_data data = {
	.freq = 0,
};

DEVICE_DT_INST_DEFINE(0, clock_control_si32_pll_init, NULL, &data, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_si32_pll_api);
