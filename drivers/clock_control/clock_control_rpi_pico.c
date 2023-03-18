/*
 * Copyright (c) 2022 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_clocks

#include <zephyr/drivers/clock_control/rpi_pico_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

/* pico-sdk includes */
#include <hardware/clocks.h>

#define CTRL_ENABLE_BITS CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS
#define FIXED_CLOCK_FREQ(node_id, child) DT_PROP(DT_CHILD(node_id, child), clock_frequency)

struct clock_control_rpi_config {
	clocks_hw_t *const clocks_regs;
};

struct clock_control_rpi_data {
	struct rpi_pico_clk_setup clocks_data[RPI_PICO_CLOCK_COUNT];
	uint32_t frequencies[RPI_PICO_CLOCK_COUNT];
};

static int rpi_clock_configure(const struct device *dev, uint32_t clk_index,
			       const struct rpi_pico_clk_setup *clk_data)
{
	struct clock_control_rpi_data *const data = dev->data;
	uint32_t divider;

	divider = (uint32_t)(((uint64_t)clk_data->source_rate << 8) / clk_data->rate);

	if (!clock_configure(clk_index, clk_data->source, clk_data->aux_source,
			     clk_data->source_rate, clk_data->rate)) {
		return -EINVAL;
	}

	data->frequencies[clk_index] =
		(uint32_t)(((uint64_t)clk_data->source_rate << 8) / divider);

	return 0;
}

static int rpi_is_valid_clock_index(uint32_t index)
{
	if (index <= RPI_PICO_CLOCK_GPOUT3) {
		return -ENOTSUP;
	}

	if (index >= RPI_PICO_CLOCK_COUNT) {
		return -EINVAL;
	}

	return 0;
}

static int clock_control_rpi_on(const struct device *dev,
				clock_control_subsys_t sys)
{
	const struct clock_control_rpi_config *config = dev->config;
	struct clock_control_rpi_data *const data = dev->data;
	clocks_hw_t *clocks_regs = config->clocks_regs;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return ret;
	}

	if (clocks_regs->clk[clk_index].ctrl & CTRL_ENABLE_BITS) {
		return 0;
	}

	return rpi_clock_configure(dev, clk_index, &data->clocks_data[clk_index]);
}

static int clock_control_rpi_off(const struct device *dev,
				 clock_control_subsys_t sys)
{
	const struct clock_control_rpi_config *config = dev->config;
	struct clock_control_rpi_data *const data = dev->data;
	clocks_hw_t *clocks_regs = config->clocks_regs;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return ret;
	}

	clocks_regs->clk[clk_index].ctrl &= ~CTRL_ENABLE_BITS;

	data->frequencies[clk_index] = 0;

	return 0;
}

static enum clock_control_status clock_control_rpi_get_status(const struct device *dev,
							      clock_control_subsys_t sys)
{
	const struct clock_control_rpi_config *config = dev->config;
	clocks_hw_t *clocks_regs = config->clocks_regs;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (clocks_regs->clk[clk_index].ctrl & CTRL_ENABLE_BITS) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_rpi_get_rate(const struct device *dev,
				      clock_control_subsys_t sys,
				      uint32_t *rate)
{
	struct clock_control_rpi_data *data = dev->data;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return ret;
	}

	*rate = data->frequencies[clk_index];

	return 0;
}

static int clock_control_rpi_set_rate(const struct device *dev,
				      clock_control_subsys_t sys,
				      clock_control_subsys_rate_t rate)
{
	struct clock_control_rpi_data *data = dev->data;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return ret;
	}

	if ((uint32_t)rate == data->clocks_data[clk_index].rate) {
		return 0;
	}

	data->clocks_data[clk_index].rate = (uint32_t)rate;

	return rpi_clock_configure(dev, clk_index, &data->clocks_data[clk_index]);
}

static int clock_control_rpi_configure(const struct device *dev,
				       clock_control_subsys_t sys,
				       void *dev_data)
{
	struct rpi_pico_clk_setup *clock_data = (struct rpi_pico_clk_setup *)dev_data;
	struct clock_control_rpi_data *const data = dev->data;
	uint32_t clk_index;
	int ret;

	clk_index = (uint32_t)sys;
	ret = rpi_is_valid_clock_index(clk_index);
	if (ret < 0) {
		return ret;
	}

	data->clocks_data[clk_index] = *clock_data;

	return rpi_clock_configure(dev, clk_index, clock_data);
}

static int clock_control_rpi_init(const struct device *dev)
{
	const struct clock_control_rpi_config *config = dev->config;
	clocks_hw_t *clocks_regs = config->clocks_regs;

	clocks_regs->resus.ctrl = 0;

	clock_control_rpi_on(dev, (void *)RPI_PICO_CLOCK_REF);
	clock_control_rpi_on(dev, (void *)RPI_PICO_CLOCK_SYS);

	return 0;
}

static const struct clock_control_driver_api clock_control_rpi_api = {
	.on = clock_control_rpi_on,
	.off = clock_control_rpi_off,
	.get_rate = clock_control_rpi_get_rate,
	.get_status = clock_control_rpi_get_status,
	.set_rate = clock_control_rpi_set_rate,
	.configure = clock_control_rpi_configure
};

static const struct clock_control_rpi_config clock_control_rpi_config = {
	.clocks_regs = (clocks_hw_t *)DT_INST_REG_ADDR(0),
};

static struct clock_control_rpi_data clock_control_rpi_data = {
	.clocks_data = {
		[RPI_PICO_CLOCK_REF] = {
			.source = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
			.aux_source = 0,
			.source_rate = FIXED_CLOCK_FREQ(DT_NODELABEL(clocks), xtal_clk),
			.rate = DT_INST_PROP(0, ref_frequency)
		},
		[RPI_PICO_CLOCK_SYS] = {
			.source = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
			.aux_source = CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
			.source_rate =  FIXED_CLOCK_FREQ(DT_NODELABEL(clocks), sys_pll),
			.rate = DT_INST_PROP(0, sys_frequency)
		},
		[RPI_PICO_CLOCK_PERI] = {
			.source = 0,
			.aux_source = CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
			.source_rate = DT_INST_PROP(0, sys_frequency),
			.rate = DT_INST_PROP(0, peri_frequency)
		},
		[RPI_PICO_CLOCK_USB] = {
			.source = 0,
			.aux_source = CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
			.source_rate = FIXED_CLOCK_FREQ(DT_NODELABEL(clocks), usb_pll),
			.rate = DT_INST_PROP(0, usb_frequency)
		},
		[RPI_PICO_CLOCK_ADC] = {
			.source = 0,
			.aux_source = CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
			.source_rate = FIXED_CLOCK_FREQ(DT_NODELABEL(clocks), usb_pll),
			.rate = DT_INST_PROP(0, adc_frequency)
		},
		[RPI_PICO_CLOCK_RTC] = {
			.source = 0,
			.aux_source = CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
			.source_rate = FIXED_CLOCK_FREQ(DT_NODELABEL(clocks), usb_pll),
			.rate = DT_INST_PROP(0, rtc_frequency)
		}
	},
	.frequencies = {0}
};

DEVICE_DT_INST_DEFINE(0, &clock_control_rpi_init, NULL,
		      &clock_control_rpi_data, &clock_control_rpi_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &clock_control_rpi_api);
