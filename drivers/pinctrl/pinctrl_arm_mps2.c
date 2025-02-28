/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"
#include <stdint.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/drivers/gpio/gpio_cmsdk_ahb.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/clock_control/arm_clock_control.h>

/**
 * The ARM MPS2 Board has 4 GPIO controllers. These controllers
 * are responsible for pin muxing, input/output, pull-up, etc.
 *
 * All GPIO controller pins are exposed via the following sequence of pin
 * numbers:
 *   Pins  0 -  15 are for GPIO0
 *   Pins 16 -  31 are for GPIO1
 *   Pins 32 -  47 are for GPIO2
 *   Pins 48 -  51 are for GPIO3
 *
 * For the GPIO controllers configuration ARM MPS2 Board follows the
 * Arduino compliant pin out.
 */

static const struct device *const gpio_ports[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio2)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio3)),
};

typedef void (*gpio_config_func_t)(const struct device *port);

struct gpio_cmsdk_ahb_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	volatile struct gpio_cmsdk_ahb *port;
	gpio_config_func_t gpio_config_func;
	/* GPIO Clock control in Active State */
	struct arm_clock_control_t gpio_cc_as;
	/* GPIO Clock control in Sleep State */
	struct arm_clock_control_t gpio_cc_ss;
	/* GPIO Clock control in Deep Sleep State */
	struct arm_clock_control_t gpio_cc_dss;

	const struct pinctrl_dev_config *pctrl;
};

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t flags = pin->input_enable ? GPIO_INPUT : GPIO_OUTPUT;

	/* Each gpio has 16 pins, so divide by 16 to get specific gpio*/
	const struct device *gpio_dev = gpio_ports[pin->pin_num >> 4];

	/* mod 16 gets pin number, then BIT macro converts to bit mask */
	uint32_t pin_mask = BIT(pin->pin_num % 16);

	if (gpio_dev == NULL) {
		return -EINVAL;
	}
	const struct gpio_cmsdk_ahb_cfg *const cfg = gpio_dev->config;

	if (((flags & GPIO_INPUT) == 0) && ((flags & GPIO_OUTPUT) == 0)) {
		return -ENOTSUP;
	}

	/*
	 * Setup the pin direction
	 * Output Enable:
	 * 0 - Input
	 * 1 - Output
	 */
	if ((flags & GPIO_OUTPUT) != 0) {
		cfg->port->outenableset = pin_mask;
	} else {
		cfg->port->outenableclr = pin_mask;
	}

	cfg->port->altfuncclr = pin_mask;

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		if (pinctrl_configure_pin(pins++) == -ENOTSUP) {
			return -ENOTSUP;
		}
	}

	return 0;
}
