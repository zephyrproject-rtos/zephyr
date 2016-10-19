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

#if defined(CONFIG_NETWORKING_WITH_15_4_TI_CC2520)

#include <gpio.h>

static struct device *cc2520_gpio_config[CC2520_GPIO_IDX_LAST_ENTRY];

struct device **cc2520_configure_gpios(void)
{
	struct device *gpio;

	const int flags_noint_out = GPIO_DIR_OUT;
	const int flags_noint_in = GPIO_DIR_IN;
	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				  GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio = device_get_binding(CONFIG_TI_CC2520_GPIO_1_NAME);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_VREG_EN, flags_noint_out);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_RESET, flags_noint_out);

	cc2520_gpio_config[CC2520_GPIO_IDX_VREG_EN] = gpio;
	cc2520_gpio_config[CC2520_GPIO_IDX_RESET] = gpio;

	gpio = device_get_binding(CONFIG_TI_CC2520_GPIO_0_NAME);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_SFD, flags_int_in);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_FIFOP, flags_int_in);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_FIFO, flags_noint_in);
	gpio_pin_configure(gpio, CONFIG_CC2520_GPIO_CCA, flags_noint_in);

	cc2520_gpio_config[CC2520_GPIO_IDX_FIFOP] = gpio;
	cc2520_gpio_config[CC2520_GPIO_IDX_FIFO] = gpio;
	cc2520_gpio_config[CC2520_GPIO_IDX_SFD] = gpio;
	cc2520_gpio_config[CC2520_GPIO_IDX_CCA] = gpio;

	return cc2520_gpio_config;
}

#endif /* CONFIG_NETWORKING_WITH_15_4_TI_CC2520 */
