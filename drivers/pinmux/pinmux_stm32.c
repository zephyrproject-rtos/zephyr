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
#include <pinmux/pinmux_stm32.h>
#include <clock_control/stm32_clock_control.h>

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

/**
 * @brief pin setup
 *
 * @param pin  STM32PIN() encoded pin ID
 * @param func SoC specific function assignment
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
static inline int __pinmux_stm32_set(uint32_t pin, uint32_t func,
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

static int pinmux_stm32_set(struct device *dev,
				 uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	return __pinmux_stm32_set(pin, func, NULL);
}

static int pinmux_stm32_get(struct device *dev,
				 uint32_t pin, uint32_t *func)
{
	return -ENOTSUP;
}

static int pinmux_stm32_input(struct device *dev,
				uint32_t pin,
				uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_stm32_pullup(struct device *dev,
				uint32_t pin,
				uint8_t func)
{
	return -ENOTSUP;
}

static struct pinmux_driver_api pinmux_stm32_api = {
	.set = pinmux_stm32_set,
	.get = pinmux_stm32_get,
	.pullup = pinmux_stm32_pullup,
	.input = pinmux_stm32_input,
};

/**
 * @brief setup pins according to their assignments
 *
 * @param pinconf  board pin configuration array
 * @param pins     array size
 */
static inline void __setup_pins(const struct pin_config *pinconf,
				size_t pins)
{
	struct device *clk;
	int i;

	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	for (i = 0; i < pins; i++) {
		__pinmux_stm32_set(pinconf[i].pin_num,
				pinconf[i].mode, clk);
	}
}

int pinmux_stm32_init(struct device *port)
{
	size_t pins = 0;
	const struct pin_config *pinconf;

	pinconf = stm32_board_get_pinconf(&pins);

	if (pins != 0) {
		/* configure pins */
		__setup_pins(pinconf, pins);
	}

	port->driver_api = &pinmux_stm32_api;
	return 0;
}

static struct pinmux_config pinmux_stm32_cfg = {
#ifdef CONFIG_SOC_STM32F1X
	.base_address = GPIO_PORTS_BASE,
#endif
};

/**
 * @brief device init
 *
 * Device priority set to 2, so that we come after clock_control and
 * before any other devices
 */
DEVICE_INIT(pinmux_stm32, STM32_PINMUX_NAME, &pinmux_stm32_init,
	    NULL, &pinmux_stm32_cfg,
	    PRIMARY, CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
