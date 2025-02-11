/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/pinctrl/max32-pinctrl.h>
#include <zephyr/dt-bindings/gpio/adi-max32-gpio.h>
#include <zephyr/drivers/pinctrl.h>

#include <gpio.h>

#define ADI_MAX32_GET_PORT_ADDR_OR_NONE(nodelabel)                                                 \
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),                                        \
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/** GPIO port addresses */
static const uint32_t gpios[] = {
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio0)
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio1)
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio2)
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio3)
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio4)
	ADI_MAX32_GET_PORT_ADDR_OR_NONE(gpio5)
};

static int pinctrl_configure_pin(pinctrl_soc_pin_t soc_pin)
{
	uint32_t port;
	uint32_t pin;
	uint32_t afx;
	int pincfg;
	mxc_gpio_cfg_t gpio_cfg;

	port = MAX32_PINMUX_PORT(soc_pin.pinmux);
	pin = MAX32_PINMUX_PIN(soc_pin.pinmux);
	afx = MAX32_PINMUX_MODE(soc_pin.pinmux);
	pincfg = soc_pin.pincfg;

	if (gpios[port] == 0) {
		return -EINVAL;
	}

	gpio_cfg.port = (mxc_gpio_regs_t *)gpios[port];
	gpio_cfg.mask = BIT(pin);

	if (pincfg & BIT(MAX32_BIAS_PULL_UP_SHIFT)) {
		gpio_cfg.pad = MXC_GPIO_PAD_PULL_UP;
	} else if (pincfg & BIT(MAX32_BIAS_PULL_DOWN_SHIFT)) {
		gpio_cfg.pad = MXC_GPIO_PAD_PULL_DOWN;
	} else {
		gpio_cfg.pad = MXC_GPIO_PAD_NONE;
	}

	if (pincfg & BIT(MAX32_INPUT_ENABLE_SHIFT)) {
		gpio_cfg.func = MXC_GPIO_FUNC_IN;
	} else if (pincfg & BIT(MAX32_OUTPUT_ENABLE_SHIFT)) {
		gpio_cfg.func = MXC_GPIO_FUNC_OUT;
	} else {
		/* Add +1 to index match */
		gpio_cfg.func = (mxc_gpio_func_t)(afx + 1);
	}

	if (pincfg & BIT(MAX32_POWER_SOURCE_SHIFT)) {
		gpio_cfg.vssel = MXC_GPIO_VSSEL_VDDIOH;
	} else {
		gpio_cfg.vssel = MXC_GPIO_VSSEL_VDDIO;
	}

	switch (pincfg & MAX32_GPIO_DRV_STRENGTH_MASK) {
	case MAX32_GPIO_DRV_STRENGTH_1:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_1;
		break;
	case MAX32_GPIO_DRV_STRENGTH_2:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_2;
		break;
	case MAX32_GPIO_DRV_STRENGTH_3:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_3;
		break;
	default:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_0;
		break;
	}

	if (MXC_GPIO_Config(&gpio_cfg) != 0) {
		return -ENOTSUP;
	}

	if (pincfg & BIT(MAX32_OUTPUT_ENABLE_SHIFT)) {
		if (pincfg & BIT(MAX32_OUTPUT_HIGH_SHIFT)) {
			MXC_GPIO_OutSet(gpio_cfg.port, BIT(pin));
		} else {
			MXC_GPIO_OutClr(gpio_cfg.port, BIT(pin));
		}
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	int ret;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = pinctrl_configure_pin(*pins++);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
