/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_pinctrl

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/xmc4xxx-pinctrl.h>

#include <xmc_gpio.h>

#define GPIO_REG_SIZE 0x100

static int pinctrl_configure_pin(const pinctrl_soc_pin_t pinmux)
{
	int port_id, pin, alt_fun, hwctrl;
	XMC_GPIO_CONFIG_t pin_cfg = {0};
	XMC_GPIO_PORT_t *gpio_port;

	port_id = XMC4XXX_PINMUX_GET_PORT(pinmux);
	if (port_id >= DT_INST_REG_SIZE(0) / GPIO_REG_SIZE) {
		return -EINVAL;
	}

	pin = XMC4XXX_PINMUX_GET_PIN(pinmux);

	if (XMC4XXX_PINMUX_GET_PULL_DOWN(pinmux)) {
		pin_cfg.mode = XMC_GPIO_MODE_INPUT_PULL_DOWN;
	}

	if (XMC4XXX_PINMUX_GET_PULL_UP(pinmux)) {
		pin_cfg.mode = XMC_GPIO_MODE_INPUT_PULL_UP;
	}

	if (XMC4XXX_PINMUX_GET_INV_INPUT(pinmux)) {
		pin_cfg.mode |= 0x4;
	}

	if (XMC4XXX_PINMUX_GET_OPEN_DRAIN(pinmux)) {
		pin_cfg.mode = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN;
	}

	if (XMC4XXX_PINMUX_GET_PUSH_PULL(pinmux)) {
		pin_cfg.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL;
	}

	alt_fun = XMC4XXX_PINMUX_GET_ALT(pinmux);
	pin_cfg.mode |= alt_fun << PORT0_IOCR0_PC0_Pos;

	/* only has effect if mode is push_pull */
	if (XMC4XXX_PINMUX_GET_OUT_HIGH(pinmux)) {
		pin_cfg.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH;
	}

	/* only has effect if mode is push_pull */
	if (XMC4XXX_PINMUX_GET_OUT_LOW(pinmux)) {
		pin_cfg.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW;
	}

	/* only has effect if mode is push_pull */
	pin_cfg.output_strength = XMC4XXX_PINMUX_GET_DRIVE(pinmux);

	gpio_port = (XMC_GPIO_PORT_t *)((uint32_t)DT_INST_REG_ADDR(0) + port_id * GPIO_REG_SIZE);
	XMC_GPIO_Init(gpio_port, pin, &pin_cfg);

	hwctrl = XMC4XXX_PINMUX_GET_HWCTRL(pinmux);
	if (hwctrl) {
		XMC_GPIO_SetHardwareControl(gpio_port, pin, hwctrl);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		int ret = pinctrl_configure_pin(*pins++);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
