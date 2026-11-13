/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32f0_rcc

#include <soc.h>
#include <py32f0xx_ll_rcc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/dt-bindings/clock/puya_py32f0_clock.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

static bool py32_clock_valid(const struct py32_pclken *clk)
{
	return clk != NULL && clk->bus >= PY32_PERIPH_BUS_MIN && clk->bus <= PY32_PERIPH_BUS_MAX &&
	       (clk->bus % 4U) == 0U;
}

static int py32_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct py32_pclken *clk = sys;
	unsigned int key;
	uintptr_t reg;

	ARG_UNUSED(dev);
	if (!py32_clock_valid(clk)) {
		return -EINVAL;
	}
	reg = DT_INST_REG_ADDR(0) + clk->bus;
	key = irq_lock();
	sys_write32(sys_read32(reg) | clk->enr, reg);
	/* Read back to ensure the clock is active before accessing the peripheral. */
	(void)sys_read32(reg);
	irq_unlock(key);
	return 0;
}

static int py32_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct py32_pclken *clk = sys;
	unsigned int key;
	uintptr_t reg;

	ARG_UNUSED(dev);
	if (!py32_clock_valid(clk)) {
		return -EINVAL;
	}
	reg = DT_INST_REG_ADDR(0) + clk->bus;
	key = irq_lock();
	sys_write32(sys_read32(reg) & ~clk->enr, reg);
	irq_unlock(key);
	return 0;
}

static int py32_clock_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(dev);
	if (!py32_clock_valid(sys)) {
		return -EINVAL;
	}
	*rate = DT_INST_PROP(0, clock_frequency);
	return 0;
}

static int py32_clock_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() == 0U) {
	}
	LL_RCC_SetHSIDiv(LL_RCC_HSI_DIV_1);
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS) {
	}
	SystemCoreClock = DT_INST_PROP(0, clock_frequency);
	return 0;
}

static DEVICE_API(clock_control, py32_clock_api) = {
	.on = py32_clock_on,
	.off = py32_clock_off,
	.get_rate = py32_clock_get_rate,
};

DEVICE_DT_INST_DEFINE(0, py32_clock_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &py32_clock_api);
