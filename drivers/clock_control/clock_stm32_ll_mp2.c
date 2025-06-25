/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *) sub_system;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_CLOCK_PERIPH_MIN, STM32_CLOCK_PERIPH_MAX)) {
		/* Attempt to change a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *) sub_system;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_CLOCK_PERIPH_MIN, STM32_CLOCK_PERIPH_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_PERIPH_USART1:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART1_CLKSOURCE);
		break;
	case STM32_CLOCK_PERIPH_USART2:
	case STM32_CLOCK_PERIPH_UART4:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART24_CLKSOURCE);
		break;
	case STM32_CLOCK_PERIPH_USART3:
	case STM32_CLOCK_PERIPH_UART5:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART35_CLKSOURCE);
		break;
	case STM32_CLOCK_PERIPH_USART6:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_USART6_CLKSOURCE);
		break;
	case STM32_CLOCK_PERIPH_UART7:
	case STM32_CLOCK_PERIPH_UART8:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART78_CLKSOURCE);
		break;
	case STM32_CLOCK_PERIPH_UART9:
		*rate = LL_RCC_GetUARTClockFreq(LL_RCC_UART9_CLKSOURCE);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc), stm32_clock_control_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &stm32_clock_control_api);
