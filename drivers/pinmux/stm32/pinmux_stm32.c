/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief
 *
 * A common driver for STM32 pinmux. Each SoC must implement a SoC
 * specific part of the driver.
 */

#include <nanokernel.h>
#include <device.h>
#include <soc.h>
#include "pinmux.h"
#include <pinmux.h>
#include <gpio/gpio_stm32.h>
#include <clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

/**
 * @brief enable IO port clock
 *
 * @param port I/O port ID
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
static int enable_port(uint32_t port, struct device *clk)
{
	clock_control_subsys_t subsys = stm32_get_port_clock(port);

	/* enable port clock */
	if (!clk) {
		clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	}

	return clock_control_on(clk, subsys);
}

static int stm32_pin_configure(int pin, int func)
{
	/* determine IO port registers location */
	uint32_t offset = STM32_PORT(pin) * GPIO_REG_SIZE;
	uint8_t *port_base = (uint8_t *)(GPIO_PORTS_BASE + offset);

	/* not much here, on STM32F10x the alternate function is
	 * controller by setting up GPIO pins in specific mode.
	 */
	return stm32_gpio_configure((uint32_t *)port_base, STM32_PIN(pin), func);
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
int _pinmux_stm32_set(uint32_t pin, uint32_t func,
				struct device *clk)
{
	int config;

	/* make sure to enable port clock first */
	if (enable_port(STM32_PORT(pin), clk)) {
		return -EIO;
	}

	/* determine config for alternate function */
	config = stm32_get_pin_config(pin, func);

	return stm32_pin_configure(pin, config);
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
				  pinconf[i].mode, clk);
	}
}
