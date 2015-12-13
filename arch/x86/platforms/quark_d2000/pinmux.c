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

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
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

#define PINMUX_SELECT_REGISTER(base, reg_offset) \
	(base + PINMUX_SELECT_OFFSET + (reg_offset << 2))

/*
 * A little decyphering of what is going on here:
 *
 * Each pinmux register rperesents a bank of 16 pins, 2 bits per pin for a total
 * of four possible settings per pin.
 *
 * The first argument to the macro is name of the uint32_t's that is being used
 * to contain the bit patterns for all the configuration registers.  The pin
 * number divided by 16 selects the correct register bank based on the pin
 * number.
 *
 * The pin number % 16 * 2 selects the position within the register bank for the
 * bits controlling the pin.
 *
 * All but the lower two bits of the config values are masked off to ensure
 * that we don't inadvertently affect other pins in the register bank.
 */
#define PIN_CONFIG(A, _pin, _func) \
	(A[((_pin) / 16)] |= ((0x3 & (_func)) << (((_pin) % 16) * 2)))

#define PINMUX_MAX_REGISTERS 2

/***************************
 * PINMUX mapping
 *
 * The following lines detail the possible options for the pinmux and their
 * associated pins and ball points.
 * This is the full pinmap that we have available on the board for configuration
 * including the ball position and the various modes that can be set.  In the
 * _pinmux_defaults we do not spend any time setting values that are using mode
 * A as the hardware brings up all devices by default in mode A.
 */
	/* pin, ball, mode A, mode B,      mode C       */
	/*  0   F00, gpio_0,  ai_0,        spi_m_ss0    */
	/*  1   F01, gpio_1,  ai_1,        spi_m_ss1    */
	/*  2   F02, gpio_2,  ai_2,        spi_m_ss2    */
	/*  3   F03, gpio_3,  ai_3,        spi_m_ss3    */
	/*  4   F04, gpio_4,  ai_4,        rtc_clk_out  */
	/*  5   F05, gpio_5,  ai_5,        sys_clk_out  */
	/*  6   F06, gpio_6,  ai_6,        i2c_scl      */
	/*  7   F07, gpio_7,  ai_7,        i2c_sda      */
	/*  8   F08, gpio_8,  ai_8,        spi_s_sclk   */
	/*  9   F09, gpio_9,  ai_9,        spi_s_sdin   */
	/* 10   F10, gpio_10, ai_10,       spi_s_sdout  */
	/* 11   F11, gpio_11, ai_11,       spi_s_scs    */
	/* 12   F12, gpio_12, ai_12,       uart_a_txd   */
	/* 13   F13, gpio_13, ai_13,       uart_a_rxd   */
	/* 14   F14, gpio_14, ai_14,       uart_a_rts   */
	/* 15   F15, gpio_15, ai_15,       uart_a_cts   */
	/* 16   F16, gpio_16, ai_16,       spi_m_sclk   */
	/* 17   F17, gpio_17, ai_17,       spi_m_mosi   */
	/* 18   F18, gpio_18, ai_18,       spi_m_miso   */
	/* 19   F19, tdo,     gpio_19,     pwm0         */
	/* 20   F20, trst_n,  gpio_20,     uart_b_txd   */
	/* 21   F21, tck,     gpio_21,     uart_b_rxd   */
	/* 22   F22, tms,     gpio_22,     uart_b_rts   */
	/* 23   F23, tdi,     gpio_23,     uart_b_cts   */
	/* 24   F24, gpio_24, lpd_sig_out, pwm1         */

/******** End PINMUX mapping **************************/

static void _pinmux_defaults(uint32_t base)
{
	uint32_t mux_config[PINMUX_MAX_REGISTERS] = { 0, 0 };
	int i = 0;

	PIN_CONFIG(mux_config,  0, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config,  3, PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  4, PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  6, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config,  7, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 12, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 13, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 14, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 15, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 16, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 17, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 18, PINMUX_FUNC_C);

	for (i = 0; i < PINMUX_MAX_REGISTERS; i++) {
		PRINT("PINMUX: configuring register i=%d reg=%x", i,
		      mux_config[i]);
		sys_write32(mux_config[i], PINMUX_SELECT_REGISTER(base, i));
	}

}


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

#ifdef CONFIG_PINMUX_DEV
static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	struct pinmux_config * const pmux = dev->config->config_info;
	/*
	 * the registers are 32-bit wide, but each pin requires 2 bits
	 * to set the mode (A, B, C, or D).  As such we only get 16
	 * pins per register... this can be accomplished by right shifting by 4.
	 * Only in this case we are using it for a byte offset, so we are only
	 * going to shift by 2 (avoids the multiplication).
	 */
	uint32_t register_offset = (pin >> 4);
	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	volatile uint32_t *mux_register =
		(uint32_t *)PINMUX_SELECT_REGISTER(pmux->base_address, register_offset);

	/*
	 * Finally grab the pin offset within the register
	 */
	uint32_t pin_no = pin % 16;

	/*
	 * The 2-bits is for the mode of each pin.  The value 2 repesents the
	 * bits needed for each pin's mode in the PMUX_SEL register.
	 */
	uint32_t pin_mask = MASK_2_BITS << (pin_no << 1);
	uint32_t mode_mask = func << (pin_no << 1);

	(*(mux_register)) = ((*(mux_register)) & ~pin_mask) | mode_mask;

	return DEV_OK;
}

static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	struct pinmux_config * const pmux = dev->config->config_info;
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
		(uint32_t *)PINMUX_SELECT_REGISTER(pmux->base_address, register_offset);

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

	return DEV_OK;
}
#else
static uint32_t pinmux_dev_set(struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __func__);

	return DEV_NOT_CONFIG;
}

static uint32_t pinmux_dev_get(struct device *dev, uint32_t pin, uint8_t *func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	PRINT("ERROR: %s is not enabled", __func__);

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

	port->driver_api = &api_funcs;
	_pinmux_defaults(pmux->base_address);

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
