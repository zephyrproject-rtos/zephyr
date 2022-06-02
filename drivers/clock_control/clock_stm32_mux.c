/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2022, Linaro Ltd
 *
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <zephyr/logging/log.h>
#include <soc.h>

#define DT_DRV_COMPAT st_stm32_clock_mux

LOG_MODULE_REGISTER(clock_mux, CONFIG_CLOCK_CONTROL_LOG_LEVEL);


struct stm32_clk_mux_config {
	const struct stm32_pclken pclken;
};

static int stm32_clk_mux_init(const struct device *dev)
{
	const struct stm32_clk_mux_config *cfg = dev->config;

	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &cfg->pclken) != 0) {
		LOG_ERR("Could not enable clock mux");
		return -EIO;
	}

	return 0;
}

#define STM32_MUX_CLK_INIT(id)						\
									\
static const struct stm32_clk_mux_config stm32_clk_mux_cfg_##id = {	\
	.pclken = STM32_INST_CLOCK_INFO(id, 0)				\
};									\
									\
DEVICE_DT_INST_DEFINE(id, &stm32_clk_mux_init, NULL,			\
		      NULL, &stm32_clk_mux_cfg_##id,			\
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_MUX_CLK_INIT)
