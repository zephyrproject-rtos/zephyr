/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_rcc

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <ch32_rcc.h>

#define WCH_RCC_CLOCK_ID_OFFSET(id) (((id) >> 5) & 0xFF)
#define WCH_RCC_CLOCK_ID_BIT(id)    ((id) & 0x1F)

struct clock_control_wch_rcc_config {
	RCC_TypeDef *regs;
};

static int clock_control_wch_rcc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_wch_rcc_config *config = dev->config;
	RCC_TypeDef *regs = config->regs;
	uint8_t id = (uintptr_t)sys;

	*(&regs->AHBPCENR + WCH_RCC_CLOCK_ID_OFFSET(id)) |= 1 << WCH_RCC_CLOCK_ID_BIT(id);
	return 0;
}

static int clock_control_wch_rcc_get_rate(const struct device *dev, clock_control_subsys_t sys,
					  uint32_t *rate)
{
	const struct clock_control_wch_rcc_config *config = dev->config;
	RCC_TypeDef *regs = config->regs;
	uint32_t cfgr0 = regs->CFGR0;
	uint32_t sysclk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	uint32_t ahbclk = sysclk;

	if ((cfgr0 & RCC_HPRE_3) != 0) {
		/* The range 0b1000 divides by a power of 2, where 0b1000 is /2, 0b1001 is /4, etc.
		 */
		ahbclk /= 2 << ((cfgr0 >> 4) & 0x07);
	} else {
		/* The range 0b0nnn divides by n + 1, where 0b0000 is /1, 0b001 is /2, etc. */
		ahbclk /= ((cfgr0 >> 4) & 0x07) + 1;
	}

	/* The datasheet says that AHB == APB1 == APB2, but the registers imply that APB1 and APB2
	 * can be divided from the AHB clock. Assume that the clock tree diagram is correct and
	 * always return AHB.
	 */
	*rate = ahbclk;
	return 0;
}

static struct clock_control_driver_api clock_control_wch_rcc_api = {
	.on = clock_control_wch_rcc_on,
	.get_rate = clock_control_wch_rcc_get_rate,
};

static int clock_control_wch_rcc_init(const struct device *dev)
{
	return 0;
}

#define CLOCK_CONTROL_WCH_RCC_INIT(idx)                                                            \
	static const struct clock_control_wch_rcc_config clock_control_wch_rcc_##idx##_config = {  \
		.regs = (RCC_TypeDef *)DT_INST_REG_ADDR(idx),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, clock_control_wch_rcc_init, NULL, NULL,                         \
			      &clock_control_wch_rcc_##idx##_config, PRE_KERNEL_1,                 \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_wch_rcc_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_WCH_RCC_INIT)
