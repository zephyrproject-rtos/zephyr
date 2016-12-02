/*
 * Copyright (c) 2015 Intel Corporation
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

#include <nanokernel.h>
#include "board.h"
#include <uart.h>
#include <device.h>
#include <init.h>

#if defined(CONFIG_TI_CC2520_LEGACY) || \
	defined(CONFIG_TI_CC2520) || \
	defined(CONFIG_TI_CC2520_RAW)

#include <ieee802154/cc2520.h>
#include <gpio.h>

static struct cc2520_gpio_configuration cc2520_gpios[CC2520_GPIO_IDX_MAX] = {
	{ .dev = NULL, .pin = CC2520_GPIO_VREG_EN, },
	{ .dev = NULL, .pin = CC2520_GPIO_RESET, },
	{ .dev = NULL, .pin = CC2520_GPIO_FIFO, },
	{ .dev = NULL, .pin = CC2520_GPIO_CCA, },
	{ .dev = NULL, .pin = CC2520_GPIO_SFD, },
	{ .dev = NULL, .pin = CC2520_GPIO_FIFOP, },
};

struct cc2520_gpio_configuration *cc2520_configure_gpios(void)
{
	const int flags_noint_out = GPIO_DIR_OUT;
	const int flags_noint_in = GPIO_DIR_IN;
	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				  GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	struct device *gpio;

	gpio = device_get_binding(CONFIG_TI_CC2520_GPIO_1_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_VREG_EN].pin,
			   flags_noint_out);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_RESET].pin,
			   flags_noint_out);

	cc2520_gpios[CC2520_GPIO_IDX_VREG_EN].dev = gpio;
	cc2520_gpios[CC2520_GPIO_IDX_RESET].dev = gpio;

	gpio = device_get_binding(CONFIG_TI_CC2520_GPIO_0_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_SFD].pin,
			   flags_int_in);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_FIFOP].pin,
			   flags_int_in);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_FIFO].pin,
			   flags_noint_in);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_CCA].pin,
			   flags_noint_in);

	cc2520_gpios[CC2520_GPIO_IDX_FIFOP].dev = gpio;
	cc2520_gpios[CC2520_GPIO_IDX_FIFO].dev = gpio;
	cc2520_gpios[CC2520_GPIO_IDX_SFD].dev = gpio;
	cc2520_gpios[CC2520_GPIO_IDX_CCA].dev = gpio;

	return cc2520_gpios;
}

#endif /* CONFIG_TI_CC2520_LEGACY || CONFIG_TI_CC2520 || CONFIG_TI_CC2520_RAW */
