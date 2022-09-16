/*
 * Copyright (c) 2021 Casper Meijn <casper@meijn.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pine64_pinetime_key_out);

#define KEY_OUT_NODE DT_PATH(key_out)

#if DT_NODE_HAS_STATUS(KEY_OUT_NODE, okay)

static const struct gpio_dt_spec key_out = GPIO_DT_SPEC_GET(KEY_OUT_NODE, gpios);

static int pinetime_key_out_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	int ret;

	if (!device_is_ready(key_out.port)) {
		LOG_ERR("key out gpio device %s is not ready",
			key_out.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&key_out, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		LOG_ERR("failed to configure %s pin %d (err %d)",
			key_out.port->name, key_out.pin, ret);
		return ret;
	}

	return 0;
}

SYS_INIT(pinetime_key_out_init, POST_KERNEL, 99);
#endif
