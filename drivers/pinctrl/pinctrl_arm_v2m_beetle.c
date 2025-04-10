/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/drivers/gpio/gpio_cmsdk_ahb.h>

#define EXPANSION_SHIELD_POWER_ENABLE_MASK	BIT(15)

static const struct device *const gpio_ports[] = {DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0)),
						  DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1))};

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t flags = pin->input_enable ? GPIO_INPUT : GPIO_OUTPUT;

	/* Each gpio has 16 pins, so divide by 16 to get specific gpio*/
	const struct device *gpio_dev = gpio_ports[pin->pin_num >> 4];

	return cmsdk_ahb_gpio_config(gpio_dev, pin->pin_num % 16, flags);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		if (pinctrl_configure_pin(pins++) == -ENOTSUP) {
			return -ENOTSUP;
		}
	}

	const struct gpio_cmsdk_ahb_cfg *const cfg = gpio_ports[1]->config;
	/* Set the ARD_PWR_EN GPIO1[15] as an output */
	cfg->port->outenableset |= EXPANSION_SHIELD_POWER_ENABLE_MASK;
	/* Set on 3v3 (for ARDUINO HDR compliance) */
	cfg->port->data |= EXPANSION_SHIELD_POWER_ENABLE_MASK;

	return 0;
}
