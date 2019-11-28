/** @file
 * @brief Modem pin setup for modem context driver
 *
 * GPIO-based pin handling for the modem context driver
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <device.h>
#include <drivers/gpio.h>

#include "modem_context.h"

int modem_pin_read(struct modem_context *ctx, u32_t pin)
{
	int ret = 0;
	u32_t value = 0;

	if (pin < 0 || pin >= ctx->pins_len) {
		return -ENODEV;
	}

	ret = gpio_pin_read(ctx->pins[pin].gpio_port_dev, ctx->pins[pin].pin,
			    &value);
	if (ret < 0) {
		return ret;
	}

	return (int)value;
}

int modem_pin_write(struct modem_context *ctx, u32_t pin, u32_t value)
{
	if (pin < 0 || pin >= ctx->pins_len) {
		return -ENODEV;
	}

	return gpio_pin_write(ctx->pins[pin].gpio_port_dev, ctx->pins[pin].pin,
			      value);
}

int modem_pin_config(struct modem_context *ctx, u32_t pin, int flags)
{
	if (pin < 0 || pin >= ctx->pins_len) {
		return -ENODEV;
	}

	return gpio_pin_configure(ctx->pins[pin].gpio_port_dev,
				  ctx->pins[pin].pin, flags);
}

int modem_pin_init(struct modem_context *ctx)
{
	int i, ret;

	/* setup port devices and pin directions */
	for (i = 0; i < ctx->pins_len; i++) {
		ctx->pins[i].gpio_port_dev =
				device_get_binding(ctx->pins[i].dev_name);
		if (!ctx->pins[i].gpio_port_dev) {
			return -ENODEV;
		}

		ret = gpio_pin_configure(ctx->pins[i].gpio_port_dev,
					 ctx->pins[i].pin,
					 ctx->pins[i].init_flags);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
