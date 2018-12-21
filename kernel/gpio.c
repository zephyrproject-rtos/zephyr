/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>

int _impl_gpio_get_pending_int(struct device *dev)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

int _impl_gpio_config(struct device *port, int access_op,
		      u32_t pin, int flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->config(port, access_op, pin, flags);
}

int _impl_gpio_write(struct device *port, int access_op,
		     u32_t pin, u32_t value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->write(port, access_op, pin, value);
}

int _impl_gpio_read(struct device *port, int access_op,
		    u32_t pin, u32_t *value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->read(port, access_op, pin, value);
}

int _impl_gpio_enable_callback(struct device *port,
			       int access_op, u32_t pin)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->enable_callback(port, access_op, pin);
}

int _impl_gpio_disable_callback(struct device *port,
				int access_op, u32_t pin)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->disable_callback(port, access_op, pin);
}
