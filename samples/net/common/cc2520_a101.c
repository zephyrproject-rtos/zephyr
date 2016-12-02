/* cc2520_a101.c - Arduino 101 board setup for cc2520 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#include <ieee802154/cc2520.h>
#include <gpio.h>

#define CC2520_GPIO_DEV_NAME CONFIG_GPIO_QMSI_0_NAME

#define CC2520_GPIO_VREG_EN	16  /* PIN 50, IO8 */
#define CC2520_GPIO_RESET	20  /* PIN 54, IO7 */
#define CC2520_GPIO_FIFO	19  /* PIN 53, IO4 */
#define CC2520_GPIO_CCA		18  /* PIN 52, IO2 */
#define CC2520_GPIO_SFD		15  /* PIN 49, IO5 */
#define CC2520_GPIO_FIFOP	17  /* PIN 51, IO3 */

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
	struct device *gpio;
	int i;

	gpio = device_get_binding(CC2520_GPIO_DEV_NAME);
	if (!gpio) {
		return NULL;
	}

	for (i = 0; i < CC2520_GPIO_IDX_MAX; i++) {
		int flags;

		if (i >= 0 && i < CC2520_GPIO_IDX_FIFO) {
			flags = GPIO_DIR_OUT;
		} else if (i < CC2520_GPIO_IDX_SFD) {
			flags = GPIO_DIR_IN;
		} else {
			flags = (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				 GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
		}

		gpio_pin_configure(gpio, cc2520_gpios[i].pin, flags);
		cc2520_gpios[i].dev = gpio;
	}

	return cc2520_gpios;
}
