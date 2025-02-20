/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2024, Joakim Andersson
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_clock_mco)
#define DT_DRV_COMPAT st_stm32_clock_mco
#define HAS_PRESCALER 1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_clock_mco)
#define DT_DRV_COMPAT st_stm32f1_clock_mco
#endif

struct stm32_mco_config {
	const struct pinctrl_dev_config *pcfg;
#if defined(HAS_PRESCALER)
	uint32_t prescaler;
#endif
	/* clock subsystem driving this peripheral */
	const struct stm32_pclken pclken[1];
};

static int stm32_mco_init(const struct device *dev)
{
	const struct stm32_mco_config *config = dev->config;
	const struct stm32_pclken *pclken = &config->pclken[0];
	int err;

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	/* MCO source */
	sys_clear_bits(
		DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		STM32_DT_CLKSEL_MASK_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));
	sys_set_bits(
		DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		STM32_DT_CLKSEL_VAL_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));

#if defined(HAS_PRESCALER)
	/* MCO prescaler */
	sys_clear_bits(
		DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(config->prescaler),
		STM32_DT_CLKSEL_MASK_GET(config->prescaler) <<
			STM32_DT_CLKSEL_SHIFT_GET(config->prescaler));
	sys_set_bits(
		DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(config->prescaler),
		STM32_DT_CLKSEL_VAL_GET(config->prescaler) <<
			STM32_DT_CLKSEL_SHIFT_GET(config->prescaler));
#endif

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

#define STM32_MCO_INIT(inst)                                            \
									\
PINCTRL_DT_INST_DEFINE(inst);                                           \
									\
const static struct stm32_mco_config stm32_mco_config_##inst = {        \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                   \
	.pclken = STM32_DT_INST_CLOCKS(inst),                           \
	IF_ENABLED(HAS_PRESCALER,                                       \
		(.prescaler = DT_PROP(DT_DRV_INST(inst), prescaler),))  \
};                                                                      \
									\
DEVICE_DT_INST_DEFINE(inst, stm32_mco_init, NULL,                       \
	NULL,                                                           \
	&stm32_mco_config_##inst,                                       \
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,               \
	NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_MCO_INIT);
