/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_pinmux

#include "analog.h"
#include <drivers/pinmux.h>


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

/* GPIO Pull-Up options */
#define PINMUX_B91_PULLUP_DISABLE   ((uint8_t)0u)
#define PINMUX_B91_PULLUP_10K       ((uint8_t)3u)

/* B91 PinMux config structure */
struct pinmux_b91_config {
	uint8_t pad_mul_sel;
};


/* Act as GPIO function disable */
static inline void pinmux_b91_gpio_function_disable(uint32_t pin)
{
	uint8_t bit = pin & 0xff;

	reg_gpio_en(pin) &= ~bit;
}

/* Get function value bits start position (offset) */
static inline int pinmux_b91_get_func_offset(uint32_t pin, uint8_t *offset)
{
	switch ((pin & 0x0fu) != 0u ? pin & 0x0fu : (pin & 0xf0u) >> 4u) {
	case BIT(0):
		*offset = 0u;
		break;
	case BIT(1):
		*offset = 2u;
		break;
	case BIT(2):
		*offset = 4u;
		break;
	case BIT(3):
		*offset = 6u;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* Set pin's pull-up/down resistor */
static void pinmux_b91_set_pull_up(uint32_t pin, uint8_t val)
{
	uint8_t mask = 0;
	uint8_t analog_reg = reg_pull_up_en(pin);

	if (pin & 0x11) {
		val = val;
		mask = 0xfc;
	} else if (pin & 0x22) {
		val = val << 2;
		mask = 0xf3;
	} else if (pin & 0x44) {
		val = val << 4;
		mask = 0xcf;
	} else if (pin & 0x88) {
		val = val << 6;
		mask = 0x3f;
	} else {
		return;
	}

	analog_write_reg8(analog_reg, (analog_read_reg8(analog_reg) & mask) | val);
}

/* API implementation: init */
static int pinmux_b91_init(const struct device *dev)
{
	const struct pinmux_b91_config *cfg = dev->config;

	reg_gpio_pad_mul_sel |= cfg->pad_mul_sel;

	return 0;
}

/* API implementation: set */
static int pinmux_b91_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	uint8_t mask = 0;
	uint8_t offset = 0;
	int32_t status = 0;

	/* calculate offset and mask for the func value */
	status = pinmux_b91_get_func_offset(pin, &offset);
	if (status != 0) {
		return status;
	}
	mask = (uint8_t)~(BIT(offset) | BIT(offset + 1));

	/* disable GPIO function (can be enabled back by GPIO init using GPIO driver) */
	pinmux_b91_gpio_function_disable(pin);

	/* set func value */
	reg_pin_mux(pin) = (reg_pin_mux(pin) & mask) | (func << offset);

	return status;
}

/* API implementation: get */
static int pinmux_b91_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	ARG_UNUSED(dev);

	uint8_t mask = 0u;
	uint8_t offset = 0;
	int32_t status = 0;

	/* calculate offset and mask for the func value */
	status = pinmux_b91_get_func_offset(pin, &offset);
	if (status != 0) {
		return status;
	}
	mask = (uint8_t)(BIT(offset) | BIT(offset + 1));

	/* get func value */
	*func = (reg_pin_mux(pin) & mask) >> offset;

	return status;
}

/* API implementation: pullup */
static int pinmux_b91_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	switch (func) {
	case PINMUX_PULLUP_ENABLE:
		pinmux_b91_set_pull_up(pin, PINMUX_B91_PULLUP_10K);
		break;

	case PINMUX_PULLUP_DISABLE:
		pinmux_b91_set_pull_up(pin, PINMUX_B91_PULLUP_DISABLE);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/* API implementation: input */
static int pinmux_b91_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	/* Implemented by GPIO driver */

	return -ENOTSUP;
}

/* PinMux driver APIs structure */
static const struct pinmux_driver_api pinmux_b91_api = {
	.set = pinmux_b91_set,
	.get = pinmux_b91_get,
	.pullup = pinmux_b91_pullup,
	.input = pinmux_b91_input,
};

static const struct pinmux_b91_config pinmux_b91_cfg = {
	.pad_mul_sel = DT_INST_PROP(0, pad_mul_sel)
};

/* PinMux driver registration */
DEVICE_DT_INST_DEFINE(0, pinmux_b91_init,
		      NULL, NULL, &pinmux_b91_cfg, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		      &pinmux_b91_api);
