/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
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

#include "soc.h"
#include <errno.h>
#include <device.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <gpio/gpio_stm32.h>

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

	/* ToDo: encode and return the 'real' alternate function number */
	return -EINVAL;
}
