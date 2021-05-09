/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc

#include <device.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(memc_stm32, CONFIG_MEMC_LOG_LEVEL);

struct memc_stm32_config {
	uint32_t fmc;
	struct stm32_pclken pclken;
	const struct soc_gpio_pinctrl *pinctrl;
	size_t pinctrl_len;
};

static int memc_stm32_init(const struct device *dev)
{
	const struct memc_stm32_config *config = dev->config;

	int r;
	const struct device *clk;

	/* configure pinmux */
	r = stm32_dt_pinctrl_configure(config->pinctrl, config->pinctrl_len,
				       config->fmc);
	if (r < 0) {
		LOG_ERR("FMC pinctrl setup failed (%d)", r);
		return r;
	}

	/* enable FMC peripheral clock */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	r = clock_control_on(clk, (clock_control_subsys_t *)&config->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize FMC clock (%d)", r);
		return r;
	}

	return 0;
}

static const struct soc_gpio_pinctrl pinctrl[] = ST_STM32_DT_INST_PINCTRL(0, 0);

static const struct memc_stm32_config config = {
	.fmc = DT_INST_REG_ADDR(0),
	.pclken = { .bus = DT_INST_CLOCKS_CELL(0, bus),
		    .enr = DT_INST_CLOCKS_CELL(0, bits) },
	.pinctrl = pinctrl,
	.pinctrl_len = ARRAY_SIZE(pinctrl),
};

DEVICE_DT_INST_DEFINE(0, memc_stm32_init, device_pm_control_nop, NULL,
	      &config, POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);
