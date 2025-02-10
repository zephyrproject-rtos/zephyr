/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_morse_gpio_tx

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/morse/morse_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse_gpio_tx, CONFIG_MORSE_LOG_LEVEL);

struct morse_gpio_tx_config {
	const struct gpio_dt_spec gpio;
};

int morse_gpio_tx_bit_state(const struct device *dev,
			    enum morse_bit_state state)
{
	const struct morse_gpio_tx_config *const cfg = dev->config;

	return gpio_pin_set_dt(&cfg->gpio, state);
}

static int morse_gpio_tx_init(const struct device *dev)
{
	const struct morse_gpio_tx_config *const cfg = dev->config;
	int ret;

	LOG_DBG("GPIO");
	if (!gpio_is_ready_dt(&cfg->gpio)) {
		LOG_ERR("Error: GPIO device %s is not ready", cfg->gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Error: GPIO device %s do not configure", cfg->gpio.port->name);
		return -EFAULT;
	}

	return 0;
}

static DEVICE_API(morse, morse_gpio_tx_api) = {
	.tx_bit_state = morse_gpio_tx_bit_state,
};

#define MORSE_GPIO_TX_INIT(n)							\
	static const struct morse_gpio_tx_config morse_gpio_tx_cfg_##n = {	\
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(n), gpios),		\
	};									\
	DEVICE_DT_INST_DEFINE(							\
		n, morse_gpio_tx_init,						\
		NULL,								\
		NULL,								\
		&morse_gpio_tx_cfg_##n,						\
		PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,			\
		&morse_gpio_tx_api);

DT_INST_FOREACH_STATUS_OKAY(MORSE_GPIO_TX_INIT)
