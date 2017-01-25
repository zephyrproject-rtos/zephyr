/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include "soc.h"
#include "soc_pinmux.h"
#include <device.h>
#include <misc/util.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>

static const stm32_pin_func_t pin_pa9_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PA9_USART1_TX - 1] =
		STM32L4X_PIN_CONFIG_PUSH_PULL,
};

static const stm32_pin_func_t pin_pa10_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PA10_USART1_RX - 1] =
		STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
};

static const stm32_pin_func_t pin_pa2_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PA2_USART2_TX - 1] =
		STM32L4X_PIN_CONFIG_PUSH_PULL,
};

static const stm32_pin_func_t pin_pa3_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PA3_USART2_RX - 1] =
		STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
};

static const stm32_pin_func_t pin_pb6_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PB6_I2C1_SCL - 1] =
		STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_UP,
};

static const stm32_pin_func_t pin_pb7_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PB7_I2C1_SDA - 1] =
		STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_UP,
};

static const stm32_pin_func_t pin_pb10_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PB10_USART3_TX - 1] =
		STM32L4X_PIN_CONFIG_PUSH_PULL,
};

static const stm32_pin_func_t pin_pb11_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PB11_USART3_RX - 1] =
		STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
};

static const stm32_pin_func_t pin_pa0_funcs[] = {
	[STM32L4X_PINMUX_FUNC_PA0_PWM2_CH1 - 1] =
		STM32L4X_PIN_CONFIG_PUSH_PULL,
};

/**
 * @brief pin configuration
 */
static const struct stm32_pinmux_conf pins[] = {
	STM32_PIN_CONF(STM32_PIN_PA0, pin_pa0_funcs),
	STM32_PIN_CONF(STM32_PIN_PA2, pin_pa2_funcs),
	STM32_PIN_CONF(STM32_PIN_PA3, pin_pa3_funcs),
	STM32_PIN_CONF(STM32_PIN_PA9, pin_pa9_funcs),
	STM32_PIN_CONF(STM32_PIN_PA10, pin_pa10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB6, pin_pb6_funcs),
	STM32_PIN_CONF(STM32_PIN_PB7, pin_pb7_funcs),
	STM32_PIN_CONF(STM32_PIN_PB10, pin_pb10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB11, pin_pb11_funcs),
};

int stm32_get_pin_config(int pin, int func)
{
	/* GPIO function is always available, to save space it is not
	 * listed in alternate functions array
	 */
	if (func == STM32_PINMUX_FUNC_GPIO) {
		return STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
	}

	/* analog function is another 'known' setting */
	if (func == STM32_PINMUX_FUNC_ANALOG) {
		return STM32L4X_PIN_CONFIG_ANALOG;
	}

	for (int i = 0; i < ARRAY_SIZE(pins); i++) {
		if (pins[i].pin == pin) {
			if ((func - 1) >= pins[i].nfuncs) {
				return -EINVAL;
			}

			return pins[i].funcs[func - 1];
		}
	}
	return -EINVAL;
}
