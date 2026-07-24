/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal clock-control driver for the STM32WL3x series.
 */

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/sys_bitops.h>

/* Driver definitions */
#define RCC_REG(offset) (DT_REG_ADDR(STM32_CLOCK_CONTROL_NODE) + (offset))

/* CLK_SYS at the reset configuration (from the rcc node's clock-frequency). */
#define STM32_SYSCLK_FREQ DT_PROP(STM32_CLOCK_CONTROL_NODE, clock_frequency)

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	const mem_addr_t reg = RCC_REG(pclken->bus);
	volatile uint32_t temp;

	ARG_UNUSED(dev);
	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempting to change domain clock */
		return -ENOTSUP;
	}

	sys_set_bits(reg, pclken->enr);

	/* Read back register to be blocked by RCC
	 * until peripheral clock enabling is complete
	 */
	temp = sys_read32(reg);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	const mem_addr_t reg = RCC_REG(pclken->bus);

	ARG_UNUSED(dev);
	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempting to change domain clock */
		return -ENOTSUP;
	}

	sys_clear_bits(reg, pclken->enr);

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system,
					       uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	/* All peripherals used during bring-up are clocked from CLK_SYS. */
	*rate = STM32_SYSCLK_FREQ;

	return 0;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if ((sys_read32(RCC_REG(pclken->bus)) & pclken->enr) == pclken->enr) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.get_status = stm32_clock_control_get_status,
};

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Reset config (CLK_SYS = HSI 64 MHz / 4 = 16 MHz) is left untouched.
	 * TODO: configure HSE/PLL source and CLKSYSDIV from Device Tree, and add
	 * LSI runtime measurement (as the WB0 driver does).
	 */
	return 0;
}

DEVICE_DT_DEFINE(STM32_CLOCK_CONTROL_NODE,
		 &stm32_clock_control_init,
		 NULL, NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &stm32_clock_control_api);
