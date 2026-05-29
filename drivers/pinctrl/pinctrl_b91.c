/*
 * Copyright (c) 2022 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/b91-pinctrl.h>
#include <zephyr/init.h>

#define DT_DRV_COMPAT telink_b91_pinctrl

/**
 *      GPIO Function Enable Register
 *         ADDR              PINS
 *      gpio_en:          PORT_A[0-7]
 *      gpio_en + 1*8:    PORT_B[0-7]
 *      gpio_en + 2*8:    PORT_C[0-7]
 *      gpio_en + 3*8:    PORT_D[0-7]
 *      gpio_en + 4*8:    PORT_E[0-7]
 *      gpio_en + 5*8:    PORT_F[0-7]
 */
#define reg_gpio_en(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, gpio_en) + \
						((pin >> 8) * 8)))

/**
 *      Function Multiplexer Register
 *         ADDR              PINS
 *      pin_mux:          PORT_A[0-3]
 *      pin_mux + 1:      PORT_A[4-7]
 *      pin_mux + 2:      PORT_B[0-3]
 *      pin_mux + 3:      PORT_B[4-7]
 *      pin_mux + 4:      PORT_C[0-3]
 *      pin_mux + 5:      PORT_C[4-7]
 *      pin_mux + 6:      PORT_D[0-3]
 *      pin_mux + 7:      PORT_D[4-7]
 *      pin_mux + 0x20:   PORT_E[0-3]
 *      pin_mux + 0x21:   PORT_E[4-7]
 *      pin_mux + 0x26:   PORT_F[0-3]
 *      pin_mux + 0x27:   PORT_F[4-7]
 */
#define reg_pin_mux(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, pin_mux) + \
						(((pin >> 8) < 4)  ? ((pin >> 8) * 2) : 0) +	 \
						(((pin >> 8) == 4) ? 0x20          : 0) +	 \
						(((pin >> 8) == 5) ? 0x26          : 0) +	 \
						((pin & 0x0f0)     ? 1             : 0)))

/**
 *      Pull Up resistors enable
 *          ADDR               PINS
 *      pull_up_en:         PORT_A[0-3]
 *      pull_up_en + 1:     PORT_A[4-7]
 *      pull_up_en + 2:     PORT_B[0-3]
 *      pull_up_en + 3:     PORT_B[4-7]
 *      pull_up_en + 4:     PORT_C[0-3]
 *      pull_up_en + 5:     PORT_C[4-7]
 *      pull_up_en + 6:     PORT_D[0-3]
 *      pull_up_en + 7:     PORT_D[4-7]
 *      pull_up_en + 8:     PORT_E[0-3]
 *      pull_up_en + 9:     PORT_E[4-7]
 *      pull_up_en + 10:    PORT_F[0-3]
 *      pull_up_en + 11:    PORT_F[4-7]
 */
#define reg_pull_up_en(pin) ((uint8_t)(DT_INST_REG_ADDR_BY_NAME(0, pull_up_en) + \
				       ((pin >> 8) * 2) +			 \
				       ((pin & 0xf0) ? 1 : 0)))

/* Pinctrl driver initialization */
static int pinctrl_b91_init(void)
{

	/* set pad_mul_sel register value from dts */
	reg_gpio_pad_mul_sel |= DT_INST_PROP(0, pad_mul_sel);

	return 0;
}

SYS_INIT(pinctrl_b91_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Act as GPIO function disable */
static inline void pinctrl_b91_gpio_function_disable(uint32_t pin)
{
	uint8_t bit = pin & 0xff;

	reg_gpio_en(pin) &= ~bit;
}

/* Get function value bits start position (offset) */
static inline int pinctrl_b91_get_offset(uint32_t pin, uint8_t *offset)
{
	switch (B91_PINMUX_GET_PIN_ID(pin)) {
	case B91_PIN_0:
		*offset = B91_PIN_0_FUNC_POS;
		break;
	case B91_PIN_1:
		*offset = B91_PIN_1_FUNC_POS;
		break;
	case B91_PIN_2:
		*offset = B91_PIN_2_FUNC_POS;
		break;
	case B91_PIN_3:
		*offset = B91_PIN_3_FUNC_POS;
		break;
	case B91_PIN_4:
		*offset = B91_PIN_4_FUNC_POS;
		break;
	case B91_PIN_5:
		*offset = B91_PIN_5_FUNC_POS;
		break;
	case B91_PIN_6:
		*offset = B91_PIN_6_FUNC_POS;
		break;
	case B91_PIN_7:
		*offset = B91_PIN_7_FUNC_POS;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* Set pin's function */
static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pinctrl)
{
	int status;
	uint8_t mask;
	uint8_t offset = 0;
	uint8_t pull = B91_PINMUX_GET_PULL(*pinctrl);
	uint8_t func = B91_PINMUX_GET_FUNC(*pinctrl);
	uint32_t pin = B91_PINMUX_GET_PIN(*pinctrl);
	uint8_t pull_up_en_addr = reg_pull_up_en(pin);

	/* calculate offset and mask for the func and pull values */
	status = pinctrl_b91_get_offset(pin, &offset);
	if (status != 0) {
		return status;
	}
	mask = (uint8_t) ~(BIT(offset) | BIT(offset + 1));

	/* disable GPIO function (can be enabled back by GPIO init using GPIO driver) */
	pinctrl_b91_gpio_function_disable(pin);

	/* set func value */
	func = func << offset;
	reg_pin_mux(pin) = (reg_pin_mux(pin) & mask) | func;

	/* set pull value */
	pull = pull << offset;
	analog_write_reg8(pull_up_en_addr, (analog_read_reg8(pull_up_en_addr) & mask) | pull);

	return status;
}

/* API implementation: configure_pins */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	int status = 0;

	for (uint8_t i = 0; i < pin_cnt; i++) {
		status = pinctrl_configure_pin(pins++);
		if (status < 0) {
			break;
		}
	}

	return status;
}
