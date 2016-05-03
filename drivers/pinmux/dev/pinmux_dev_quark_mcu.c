/* pinmux_dev_quark_mcu.c - general pinmux operation */

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
#include <board.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
#include "pinmux/pinmux.h"

#include "pinmux_quark_mcu.h"

#define MASK_2_BITS             0x3

static int pinmux_dev_set(struct device *dev, uint32_t pin, uint32_t func)
{
	const struct pinmux_config *pmux = dev->config->config_info;

	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... hence the math for the register mask.
	 */
	uint32_t register_offset = (pin >> 4);
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register =
		(uint32_t *)PINMUX_SELECT_REGISTER(pmux->base_address,
						   register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The value 3 is used because that is 2-bits for the mode of each
	 * pin.  The value 2 repesents the bits needed for each pin's mode.
	 */
	uint32_t pin_mask = MASK_2_BITS << (pin_no << 1);
	uint32_t mode_mask = func << (pin_no << 1);
	(*(mux_register)) = ((*(mux_register)) & ~pin_mask) | mode_mask;

	return 0;
}

static int pinmux_dev_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	const struct pinmux_config *pmux = dev->config->config_info;

	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... hence the math for the register mask.
	 */
	uint32_t register_offset = pin >> 4;
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register =
		(uint32_t *)PINMUX_SELECT_REGISTER(pmux->base_address,
						   register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The value 3 is used because that is 2-bits for the mode of each
	 * pin.  The value 2 repesents the bits needed for each pin's mode.
	 */
	uint32_t pin_mask = MASK_2_BITS << (pin_no << 1);
	uint32_t mode_mask = (*(mux_register)) & pin_mask;
	uint32_t mode = mode_mask >> (pin_no << 1);

	*func = mode;

	return 0;
}

static int pinmux_dev_pullup(struct device *dev,
				  uint32_t pin,
				  uint8_t func)
{
	const struct pinmux_config *pmux = dev->config->config_info;

	_quark_mcu_set_mux(pmux->base_address + PINMUX_PULLUP_OFFSET,
			  pin, func);

	return 0;
}
static int pinmux_dev_input(struct device *dev,
				 uint32_t pin, uint8_t func)
{
	const struct pinmux_config *pmux = dev->config->config_info;

	_quark_mcu_set_mux(pmux->base_address + PINMUX_INPUT_OFFSET, pin, func);

	return 0;
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_dev_set,
	.get = pinmux_dev_get,
	.pullup = pinmux_dev_pullup,
	.input = pinmux_dev_input
};

static int pinmux_dev_initialize(struct device *port)
{
	return 0;
}

static struct pinmux_config board_pmux = {
	.base_address = PINMUX_BASE_ADDR,
};

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME,
		    &pinmux_dev_initialize,
		    NULL, &board_pmux,
		    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
