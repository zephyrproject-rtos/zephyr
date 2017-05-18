/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include "soc.h"
#include <device.h>
#include <misc/util.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>

static const stm32_pin_func_t pin_pc6_funcs[] = {
	[STM32F7_PINMUX_FUNC_PC6_USART6_TX - 1] =
			STM32F7X_PIN_CONFIG_AF_PUSH_UP,
};

static const stm32_pin_func_t pin_pc7_funcs[] = {
	[STM32F7_PINMUX_FUNC_PC7_USART6_RX - 1] =
			STM32F7X_PIN_CONFIG_AF_PUSH_UP,
};

/**
 * @brief pin configuration
 */
static const struct stm32_pinmux_conf pins[] = {
	STM32_PIN_CONF(STM32_PIN_PC6, pin_pc6_funcs),
	STM32_PIN_CONF(STM32_PIN_PC7, pin_pc7_funcs),
};

int stm32_get_pin_config(int pin, int func)
{
	/* GPIO function is always available, to save space it is not
	 * listed in alternate functions array
	 */
	if (func == STM32_PINMUX_FUNC_GPIO) {
		return STM32F7X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
	}

	/* analog function is another 'known' setting */
	if (func == STM32_PINMUX_FUNC_ANALOG) {
		return STM32F7X_PIN_CONFIG_ANALOG;
	}

	func -= 1;

	for (int i = 0; i < ARRAY_SIZE(pins); i++) {
		if (pins[i].pin == pin) {
			if (func > pins[i].nfuncs) {
				return -EINVAL;
			}

			return pins[i].funcs[func];
		}
	}

	return -EINVAL;
}
