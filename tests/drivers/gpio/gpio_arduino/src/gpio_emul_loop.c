/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

void gpio_emul_loopback(const struct device *port_in, uint32_t pin_in,
			const struct device *port_out, uint32_t pin_out)
{
	int val;

	val = gpio_emul_output_get(port_out, pin_out);
	gpio_emul_input_set(port_in, pin_in, val);
}
