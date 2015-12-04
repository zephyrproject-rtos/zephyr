/*
 * Copyright (c) 2015 Intel Corporation.
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

/**
 * @file pinmux operations for Quark_D2000
 */

#include <stdio.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <toolchain.h>
#include "pinmux/pinmux.h"


#ifndef CONFIG_PINMUX_DEV
#define PRINT(...) { ; }
#else
#if defined(CONFIG_PRINTK)
#include <misc/printk.h>
#define PRINT printk
#elif defined(CONFIG_STDOUT_CONSOLE)
#define PRINT printf
#endif /* CONFIG_PRINTK */
#endif /*CONFIG_PINMUX_DEV */

#define MASK_2_BITS			0x3
const uint32_t PINMUX_PULLUP_OFFSET		= 0x00;
const uint32_t PINMUX_SLEW_OFFSET		= 0x10;
const uint32_t PINMUX_INPUT_ENABLE_OFFSET	= 0x20;
const uint32_t PINMUX_SELECT_OFFSET		= 0x30;

struct pin_config mux_config[CONFIG_PINMUX_NUM_PINS] = {
	/* pin, selected mode    ball, mode A, mode B, mode C */
	{ 0, PINMUX_FUNC_C }, /* F00, gpio_0, ai_0, spi_m_ss0, DBG_VISA0 */
	{ 1, PINMUX_FUNC_A }, /* F01, gpio_1, ai_1, spi_m_ss1, DBG_VISA1 */
	{ 2, PINMUX_FUNC_A }, /* F02, gpio_2, ai_2, spi_m_ss2, DBG_VISA2 */
	{ 3, PINMUX_FUNC_B }, /* F03, gpio_3, ai_3, spi_m_ss3, DBG_VISA3 */
	{ 4, PINMUX_FUNC_B }, /* F04, gpio_4, ai_4, rtc_clk_out, DBG_VISA4 */
	{ 5, PINMUX_FUNC_A }, /* F05, gpio_5, ai_5, sys_clk_out, DBG_VISA5 */
	{ 6, PINMUX_FUNC_C }, /* F06, gpio_6, ai_6, i2c_scl, DBG_VISA6 */
	{ 7, PINMUX_FUNC_C }, /* F07, gpio_7, ai_7, i2c_sda, DBG_VISA7 */
	{ 8, PINMUX_FUNC_A }, /* F08, gpio_8, ai_8, spi_s_sclk, DBG_VISA8 */
	{ 9, PINMUX_FUNC_A }, /* F09, gpio_9, ai_9, spi_s_sdin, DBG_VISA9 */
	{ 10, PINMUX_FUNC_A }, /* F10, gpio_10, ai_10, spi_s_sdout, DBG_VISA10*/
	{ 11, PINMUX_FUNC_A }, /* F11, gpio_11, ai_11, spi_s_scs, DBG_VISA11 */
	{ 12, PINMUX_FUNC_C }, /* F12, gpio_12, ai_12, uart_a_txd, DBG_VISA12 */
	{ 13, PINMUX_FUNC_C }, /* F13, gpio_13, ai_13, uart_a_rxd, DBG_VISA13 */
	{ 14, PINMUX_FUNC_C }, /* F14, gpio_14, ai_14, uart_a_rts, DBG_VISA14 */
	{ 15, PINMUX_FUNC_C }, /* F15, gpio_15, ai_15, uart_a_cts, DBG_VISA15 */
	{ 16, PINMUX_FUNC_C }, /* F16, gpio_16, ai_16, spi_m_sclk, DBG_VISA16 */
	{ 17, PINMUX_FUNC_C }, /* F17, gpio_17, ai_17, spi_m_mosi, DBG_VISA17 */
	{ 18, PINMUX_FUNC_C }, /* F18, gpio_18, ai_18, spi_m_miso, NA */
	{ 19, PINMUX_FUNC_A }, /* F19, tdo, gpio_19, pwm0, NA */
	{ 20, PINMUX_FUNC_A }, /* F20, trst_n, gpio_20, uart_b_txd, NA */
	{ 21, PINMUX_FUNC_A }, /* F21, tck, gpio_21, uart_b_rxd, NA */
	{ 22, PINMUX_FUNC_A }, /* F22, tms, gpio_22, uart_b_rts, NA */
	{ 23, PINMUX_FUNC_A }, /* F23, tdi, gpio_23, uart_b_cts, NA */
	{ 24, PINMUX_FUNC_A }, /* F24, gpio_24, lpd_sig_out, pwm1, NA */
};

static void _quark_d2000_pullup_set(uint32_t base, uint32_t pin, uint8_t func)
{
	/*
	 * The register is a single 32-bit value, with CONFIG_PINMUX_NUM_PINS
	 * bits set in it.  Each bit represents the input enable status of the
	 * pin.
	 */
	uint32_t enable_mask =  (func & 0x01) << pin;
	uint32_t pin_mask = 0x1 << pin;

	volatile uint32_t *mux_register =
		(uint32_t *)(base + PINMUX_PULLUP_OFFSET);

	(*(mux_register)) = (((*(mux_register)) & ~pin_mask) | enable_mask);
}

static void _quark_d2000_input_enable(uint32_t base, uint32_t pin, uint8_t func)
{
	/*
	 * The register is a single 32-bit value, with CONFIG_PINMUX_NUM_PINS
	 * bits set in it.  Each bit represents the input enable status of the
	 * pin.
	 */
	uint32_t enable_mask =  (func & 0x01) << pin;
	uint32_t pin_mask = 0x1 << pin;

	volatile uint32_t *mux_register =
		(uint32_t *)(base + PINMUX_INPUT_ENABLE_OFFSET);

	(*(mux_register)) = (((*(mux_register)) & ~pin_mask) | enable_mask);
}

static void _quark_d2000_select_set(uint32_t base, uint32_t pin, uint32_t mode)
{
	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... this can be accomplished by right shifting by 4.
	 * Only in this case we are using it for a byte offset, so we are only
	 * going to shift by 2 (avoids the multiplication).
	 *
	 * uint32_t register_offset = (pin / 16) * 4;
	 */
	uint32_t register_offset = (pin >> 4) * 4;
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register =
		(uint32_t *)(base + PINMUX_SELECT_OFFSET + register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The 2-bits is for the mode of each pin.  The value 2 repesents the
	 * bits needed for each pin's mode in the PMUX_SEL register.
	 */
	uint32_t pin_mask = MASK_2_BITS << (pin_no * 2);
	uint32_t mode_mask = mode << (pin_no * 2);
	(*(mux_register)) = ((*(mux_register)) & ~pin_mask) | mode_mask;
}

#ifdef CONFIG_PINMUX_DEV

static uint32_t _quark_d2000_select_get(uint32_t base, uint32_t pin)
{
	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... hence the math for the register mask.
	 */
	/* uint32_t register_offset = (pin / 16) * 4; */
	uint32_t register_offset = (pin >> 4) * 4;
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register =
		(uint32_t *)(base + PINMUX_SELECT_OFFSET + register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The value 3 is used because that is 2-bits for the mode of each
	 * pin.  The value 2 repesents the bits needed for each pin's mode.
	 */
	uint32_t pin_mask = MASK_2_BITS << (pin_no * 2);
	uint32_t mode_mask = (*(mux_register)) & pin_mask;
	uint32_t mode = mode_mask >> (pin_no * 2);

	return mode;
}


static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	struct pinmux_config * const pmux = dev->config->config_info;

	_quark_d2000_select_set(pmux->base_address, pin, func);

	return DEV_OK;
}
#else
static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __FUNC__);

	return DEV_NOT_CONFIG;
}
#endif /* CONFIG_PINMUX_DEV */


#ifdef CONFIG_PINMUX_DEV
static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	struct pinmux_config * const pmux = dev->config->config_info;
	uint32_t ret;

	ret = _quark_d2000_select_get(pmux->base_address, pin);

	*func = ret;
	return DEV_OK;
}
#else
static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __FUNC__);

	return DEV_NOT_CONFIG;
}
#endif /* CONFIG_PINMUX_DEV */

static uint32_t pinmux_pullup_set(struct device *dev, uint32_t pin,
				  uint8_t func)
{
	struct pinmux_config * const pmux = dev->config->config_info;

	_quark_d2000_pullup_set(pmux->base_address, pin, func);

	return DEV_OK;
}

static uint32_t pinmux_input_enable(struct device *dev, uint32_t pin,
				    uint8_t func)
{
	struct pinmux_config * const pmux = dev->config->config_info;

	_quark_d2000_input_enable(pmux->base_address, pin, func);

	return DEV_OK;
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_dev_set,
	.get = pinmux_dev_get,
	.pullup = pinmux_pullup_set,
	.input = pinmux_input_enable
};


int pinmux_initialize(struct device *port)
{
	struct pinmux_config *pmux = port->config->config_info;
	int i;

	port->driver_api = &api_funcs;

	for (i = 0; i < CONFIG_PINMUX_NUM_PINS; i++) {
		PRINT("PINMUX: configuring i=%d, pin=%d, mode=%d\n", i,
		      mux_config[i].pin_num, mux_config[i].mode);
		_quark_d2000_select_set(pmux->base_address,
				mux_config[i].pin_num,
				mux_config[i].mode);
	}

	/* Enable the UART RX pin to receive input */
	_quark_d2000_input_enable(pmux->base_address, 5, PINMUX_INPUT_ENABLED);

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
