/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
LOG_MODULE_REGISTER(gpio_w91);

#define DT_DRV_COMPAT telink_w91_gpio

static int gpio_w91_pin_configure(const struct device *dev,
	gpio_pin_t pin, gpio_flags_t flags)
{
	LOG_INF("GPIO configure");
	return 0;
}

static int gpio_w91_port_toggle_bits(const struct device *dev,
	gpio_port_pins_t mask)
{
	LOG_INF("GPIO toggle bits");
	return 0;
}

static int gpio_w91_init(const struct device *dev)
{
	LOG_INF("GPIO inited");
	return 0;
}

static const struct gpio_driver_api gpio_w91_api = {
	.pin_configure = gpio_w91_pin_configure,
	.port_toggle_bits = gpio_w91_port_toggle_bits,
};

#define GPIO_W91_INIT(n)                                               \
                                                                       \
	DEVICE_DT_INST_DEFINE(n, gpio_w91_init,                            \
		NULL,                                                          \
		NULL,                                                          \
		NULL,                                                          \
		POST_KERNEL,                                                   \
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                            \
		&gpio_w91_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_W91_INIT)
