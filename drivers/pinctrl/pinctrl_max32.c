/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/max32-pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <gpio/gpio_max32.h>

static const struct device *gpios[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio2)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio3)),
};

static int pinctrl_configure_pin(pinctrl_soc_pin_t soc_pin)
{
	uint32_t pinmux = soc_pin.pinmux;
	uint32_t pincfg = soc_pin.pincfg;
	uint32_t port = MAX32_PINMUX_PORT(pinmux);
	uint32_t pin = MAX32_PINMUX_PIN(pinmux);

	return gpio_max32_config_pinmux(gpios[port], pin, pinmux, pincfg);
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
