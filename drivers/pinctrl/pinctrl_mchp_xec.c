/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pinctrl

#include <drivers/gpio.h>
#include <drivers/pinctrl.h>
#include <soc.h>

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 */
static const struct device * const gpio_ports[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_000_036)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_040_076)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_100_136)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_140_176)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_200_236)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio_240_276)),
};

/** Number of GPIO ports. */
static const size_t gpio_ports_cnt = ARRAY_SIZE(gpio_ports);

/* Microchip XEC: each GPIO pin has two 32-bit control register.
 * The first 32-bit register contains all pin features except
 * slew rate and driver strength in the second control register.
 * We compute the register index from the beginning of the GPIO
 * control address space which is the same range of the PINCTRL
 * parent node.
 */

static void config_drive_slew(uint32_t port, gpio_pin_t pin, uint32_t conf)
{
	struct gpio_regs * const regs = (struct gpio_regs * const)DT_INST_REG_ADDR(0);
	uint32_t idx = (port * 32U) + pin;
	uint32_t slew = conf &
		(MCHP_XEC_OSPEEDR_MASK << MCHP_XEC_OSPEEDR_POS);
	uint32_t drvstr = conf &
		(MCHP_XEC_ODRVSTR_MASK << MCHP_XEC_ODRVSTR_POS);
	uint32_t val = 0;
	uint32_t mask = 0;

	if (slew != MCHP_XEC_OSPEEDR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_SLEW_MASK;
		if (slew == MCHP_XEC_OSPEEDR_FAST) {
			val |= MCHP_GPIO_CTRL2_SLEW_FAST;
		}
	}
	if (drvstr != MCHP_XEC_ODRVSTR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_DRV_STR_MASK;
		val |= (drvstr << MCHP_GPIO_CTRL2_DRV_STR_POS);
	}

	if (!mask) {
		return;
	}

	regs->CTRL2[idx] = (regs->CTRL2[idx] & ~mask) | (val & mask);
}

static void config_alt_func(uint32_t port, gpio_pin_t pin, uint32_t altf)
{
	struct gpio_regs * const regs = (struct gpio_regs * const)DT_INST_REG_ADDR(0);
	uint32_t idx = (port * 32U) + pin;
	uint32_t muxval = (uint32_t)((altf & MCHP_GPIO_CTRL_MUX_MASK0) <<
				     MCHP_GPIO_CTRL_MUX_POS);

	regs->CTRL[idx] = (regs->CTRL[idx] & ~(MCHP_GPIO_CTRL_MUX_MASK)) | muxval;
}

/* Translate PINCTRL pin configuration to GPIO driver pin configuration
 * and call GPIO driver to configure the pin.
 */
static int xec_config_pin(uint32_t portpin, uint32_t conf, uint32_t altf)
{
	uint32_t port = MCHP_XEC_PINMUX_PORT(portpin);
	gpio_pin_t pin = (gpio_pin_t)MCHP_XEC_PINMUX_PIN(portpin);
	gpio_flags_t flags = 0;
	uint32_t temp = 0;
	int ret = 0;

	if (port >= gpio_ports_cnt) {
		return -EINVAL;
	}

	temp = (conf & MCHP_XEC_PUPDR_MASK) >> MCHP_XEC_PUPDR_POS;
	switch (temp) {
	case MCHP_XEC_PULL_UP:
		flags |= GPIO_PULL_UP;
		break;
	case MCHP_XEC_PULL_DOWN:
		flags |= GPIO_PULL_DOWN;
		break;
	case MCHP_XEC_REPEATER:
		flags |= (GPIO_PULL_UP | GPIO_PULL_DOWN);
		break;
	default:
		break;
	}

	if ((conf >> MCHP_XEC_OTYPER_POS) & MCHP_XEC_OTYPER_MASK) {
		flags |= GPIO_LINE_OPEN_DRAIN;
	}

	temp = (conf >> MCHP_XEC_OVAL_POS) & MCHP_XEC_OVAL_MASK;
	if (temp) {
		flags |= GPIO_OUTPUT;
		if (temp == MCHP_XEC_OVAL_DRV_HIGH) {
			flags |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_INIT_LOW;
		}
	} else {
		flags |= GPIO_INPUT;
	}

	if (conf & BIT(MCHP_XEC_PIN_LOW_POWER_POS)) {
		flags = GPIO_DISCONNECTED;
	}

	ret = gpio_pin_configure(gpio_ports[port], pin, flags);
	if (ret) {
		return ret;
	}

	if (flags != GPIO_DISCONNECTED) {
		/* program slew rate and drive strength */
		config_drive_slew(port, pin, conf);

		/* program alternate function */
		config_alt_func(port, pin, altf);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t portpin, mux, cfg, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		mux = pins[i].pinmux;

		func = MCHP_XEC_PINMUX_FUNC(mux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		cfg = pins[i].pincfg;
		portpin = MEC_XEC_PINMUX_PORT_PIN(mux);

		ret = xec_config_pin(portpin, cfg, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
