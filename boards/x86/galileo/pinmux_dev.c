/* pinmux_dev_galileo.c - pinmux_dev driver for Galileo */

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

#include <nanokernel.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <pinmux.h>
#include <i2c.h>
#include <gpio.h>
#include <pwm.h>

#include <pinmux/pinmux.h>

#include "pinmux_galileo.h"

static int galileo_dev_pullup(struct device *dev,
				   uint32_t pin,
				   uint8_t func)
{
	/*
	 * Nothing to do.
	 * On Galileo the pullup operation is handled through the selection
	 * of an actual pin
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return 0;
}

static int galileo_dev_input_enable(struct device *dev,
					 uint32_t pin,
					 uint8_t func)
{
	/*
	 * Nothing to do.
	 * On Galileo select a pin for input enabling is handled through the
	 * selection of an actual pin user configuration.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return 0;
}

static int galileo_dev_set(struct device *dev,
				uint32_t pin,
				uint32_t func)
{
	if (pin > PINMUX_NUM_PINS) {
		return -EINVAL;
	}

	return _galileo_pinmux_set_pin(dev, pin, func);
}

static int galileo_dev_get(struct device *dev,
				uint32_t pin,
				uint32_t *func)
{
	if (pin > PINMUX_NUM_PINS) {
		return -EINVAL;
	}

	return _galileo_pinmux_get_pin(dev, pin, func);
}

static struct pinmux_driver_api api_funcs = {
	.set = galileo_dev_set,
	.get = galileo_dev_get,
	.pullup = galileo_dev_pullup,
	.input = galileo_dev_input_enable
};

static int pinmux_dev_galileo_initialize(struct device *port)
{
	return 0;
}

/*
 * This needs to be a level 2 or later init process due to the following
 * dependency chain:
 * 0 - I2C
 * 1 - PCA9535 and PCAL9685
 * 2 - pinmux
 */
DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME,
		    &pinmux_dev_galileo_initialize,
		    &galileo_pinmux_driver, NULL,
		    POST_KERNEL, CONFIG_PINMUX_INIT_PRIORITY,
		    &api_funcs);
