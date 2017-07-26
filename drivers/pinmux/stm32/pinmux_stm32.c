/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * A common driver for STM32 pinmux. Each SoC must implement a SoC
 * specific part of the driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <pinmux.h>
#include <gpio/gpio_stm32.h>
#include <clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

#include "pinmux.h"

static const u32_t ports_enable[STM32_PORTS_MAX] = {
	STM32_PERIPH_GPIOA,
	STM32_PERIPH_GPIOB,
	STM32_PERIPH_GPIOC,
#ifdef GPIOD_BASE
	STM32_PERIPH_GPIOD,
#endif
#ifdef GPIOE_BASE
	STM32_PERIPH_GPIOE,
#endif
#ifdef GPIOF_BASE
	STM32_PERIPH_GPIOF,
#endif
#ifdef GPIOG_BASE
	STM32_PERIPH_GPIOG,
#endif
#ifdef GPIOH_BASE
	STM32_PERIPH_GPIOH,
#endif
};

/**
 * @brief enable IO port clock
 *
 * @param port I/O port ID
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
static int enable_port(u32_t port, struct device *clk)
{
	/* enable port clock */
	if (!clk) {
		clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	}

	struct stm32_pclken pclken;

	pclken.bus = STM32_CLOCK_BUS_GPIO;
	pclken.enr = ports_enable[port];

	return clock_control_on(clk, (clock_control_subsys_t *) &pclken);
}

static int stm32_pin_configure(int pin, int func, int altf)
{
	/* determine IO port registers location */
	u32_t offset = STM32_PORT(pin) * GPIO_REG_SIZE;
	u8_t *port_base = (u8_t *)(GPIO_PORTS_BASE + offset);

	/* not much here, on STM32F10x the alternate function is
	 * controller by setting up GPIO pins in specific mode.
	 */
	return stm32_gpio_configure((u32_t *)port_base,
				    STM32_PIN(pin), func, altf);
}

/**
 * @brief pin setup
 *
 * @param pin  STM32PIN() encoded pin ID
 * @param func SoC specific function assignment
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
int _pinmux_stm32_set(u32_t pin, u32_t func,
				struct device *clk)
{
	/* make sure to enable port clock first */
	if (enable_port(STM32_PORT(pin), clk)) {
		return -EIO;
	}

	return stm32_pin_configure(pin, func, func & STM32_AFR_MASK);
}

/**
 * @brief setup pins according to their assignments
 *
 * @param pinconf  board pin configuration array
 * @param pins     array size
 */
void stm32_setup_pins(const struct pin_config *pinconf,
		      size_t pins)
{
	struct device *clk;
	int i;

	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	for (i = 0; i < pins; i++) {
		_pinmux_stm32_set(pinconf[i].pin_num,
				  pinconf[i].mode,
				  clk);
	}
}
