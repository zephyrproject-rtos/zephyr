/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"
#include <errno.h>
#include <device.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>

int stm32_get_pin_config(int pin, int func)
{
	/* GPIO function is a known setting */
	if (func == STM32_PINMUX_FUNC_GPIO) {
		return STM32F3X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
	}

	/* analog function is another 'known' setting */
	if (func == STM32_PINMUX_FUNC_ANALOG) {
		return STM32F3X_PIN_CONFIG_ANALOG;
	}

	if (func > STM32_PINMUX_FUNC_ALT_MAX) {
		return -EINVAL;
	}

	/* encode and return the 'real' alternate function number */
	return STM32_PINFUNC(func, STM32F3X_PIN_CONFIG_AF);
}
