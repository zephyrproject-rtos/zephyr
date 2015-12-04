/* pinmux.c - general pinmux operation */

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
#include <stdio.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <toolchain.h>
#include "pinmux/pinmux.h"


#ifndef CONFIG_PINMUX_DEV
#define PRINT(...) {; }
#else
#if defined(CONFIG_PRINTK)
#include <misc/printk.h>
#define PRINT printk
#elif defined(CONFIG_STDOUT_CONSOLE)
#define PRINT printf
#endif /* CONFIG_PRINTK */
#endif /*CONFIG_PINMUX_DEV */


extern struct pin_config mux_config[];

static void _quark_se_select_set(uint32_t base, uint32_t pin, uint32_t mode)
{
	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... hence the math for the register mask.
	 */
	uint32_t register_offset = (pin / 16) * 4;
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register = (uint32_t *)(base + register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The value 3 is used because that is 2-bits for the mode of each
	 * pin.  The value 2 repesents the bits needed for each pin's mode.
	 */
	uint32_t pin_mask = 3 << (pin_no * 2);
	uint32_t mode_mask = mode << (pin_no * 2);
	/* (*((volatile uint32_t *)mux_address)) = ((*mux_address) & */
	(*(mux_register)) = ((*(mux_register)) & ~pin_mask) | mode_mask;
}

#ifdef CONFIG_PINMUX_DEV

static uint32_t _quark_se_select_get(uint32_t base, uint32_t pin)
{
	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... hence the math for the register mask.
	 */
	uint32_t register_offset = (pin / 16) * 4;
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register = (uint32_t *)(base + register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The value 3 is used because that is 2-bits for the mode of each
	 * pin.  The value 2 repesents the bits needed for each pin's mode.
	 */
	uint32_t pin_mask = 3 << (pin_no * 2);
	uint32_t mode_mask = (*(mux_register)) & pin_mask;
	uint32_t mode = mode_mask >> (pin_no * 2);
	/* (*((volatile uint32_t *)mux_address)) = ((*mux_address) & */
	return mode;
}


static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	struct pinmux_config * const pmux = dev->config->config_info;

	_quark_se_select_set(pmux->base_address, pin, func);

	return DEV_OK;
}
#else
static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __FUNCTION__);

	return DEV_NOT_CONFIG;
}
#endif /* CONFIG_PINMUX_DEV */


#ifdef CONFIG_PINMUX_DEV
static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	struct pinmux_config * const pmux = dev->config->config_info;
	uint32_t ret;

	ret = _quark_se_select_get(pmux->base_address, pin);

	*func = ret;
	return DEV_OK;
}
#else
static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __FUNCTION__);

	return DEV_NOT_CONFIG;
}
#endif /* CONFIG_PINMUX_DEV */


static struct pinmux_driver_api api_funcs = {
	.set = pinmux_dev_set,
	.get = pinmux_dev_get
};


int pinmux_initialize(struct device *dev)
{
	struct device_config *dev_cfg = dev->config;
	struct pinmux_config *pmux = dev_cfg->config_info;
	int i;

	dev->driver_api = &api_funcs;

	for (i = 0; i < CONFIG_PINMUX_NUM_PINS; i++) {
		_quark_se_select_set(pmux->base_address,
			    mux_config[i].pin_num,
			    mux_config[i].mode);
	}

	return DEV_OK;
}

struct pinmux_config board_pmux = {
	.base_address = CONFIG_PINMUX_BASE,
};

DECLARE_DEVICE_INIT_CONFIG(pmux,			/* config name */
			   PINMUX_NAME,			/* driver name */
			   &pinmux_initialize,		/* init function */
			   &board_pmux);		/* config options*/

SYS_DEFINE_DEVICE(pmux, NULL, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
