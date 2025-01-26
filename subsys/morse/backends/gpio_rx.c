/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_morse_gpio_rx

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/morse/morse_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse_gpio_rx, CONFIG_MORSE_LOG_LEVEL);

struct morse_gpio_rx_data {
	const struct device *dev;
	const struct gpio_dt_spec gpio;
	struct gpio_callback irq_cb;
	morse_bit_state_cb_t rx_cb;
	const struct device *morse;
};

static int morse_gpio_rx_cb(const struct device *dev,
			    morse_bit_state_cb_t callback,
			    const struct device *morse)
{
	struct morse_gpio_rx_data *data = dev->data;

	data->rx_cb = callback;
	data->morse = morse;

	return 0;
}

static inline void morse_gpio_rx_isr_handler(const struct device *port,
					     struct gpio_callback *gpio_cb,
					     uint32_t pins)
{
	struct morse_gpio_rx_data *data = CONTAINER_OF(gpio_cb,
						       struct morse_gpio_rx_data,
						       irq_cb);
	enum morse_bit_state state = gpio_pin_get_dt(&data->gpio);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (data->rx_cb) {
		(data->rx_cb)(data->dev, state, data->morse);
	}
}

static int morse_gpio_rx_init(const struct device *dev)
{
	struct morse_gpio_rx_data *data = dev->data;
	int ret;

	data->dev = dev;

	LOG_DBG("GPIO");
	if (!gpio_is_ready_dt(&data->gpio)) {
		LOG_ERR("Error: GPIO device %s is not ready", data->gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&data->gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Error: GPIO device %s do not configure", data->gpio.port->name);
		return -EFAULT;
	}
	ret = gpio_pin_interrupt_configure_dt(&data->gpio, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Error: GPIO device %s do not configure both edge interrupt",
			data->gpio.port->name);
		return -EFAULT;
	}
	gpio_init_callback(&data->irq_cb, morse_gpio_rx_isr_handler, BIT(data->gpio.pin));
	if (gpio_add_callback(data->gpio.port, &data->irq_cb) < 0) {
		LOG_ERR("Could not set IRQ callback.");
		return -ENXIO;
	}

	return 0;
}

static DEVICE_API(morse, morse_gpio_rx_api) = {
	.rx_cb = morse_gpio_rx_cb,
};

#define MORSE_GPIO_TX_INIT(n)							\
	static struct morse_gpio_rx_data morse_gpio_rx_data_##n = {		\
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(n), gpios),		\
	};									\
	DEVICE_DT_INST_DEFINE(							\
		n, morse_gpio_rx_init,						\
		NULL,								\
		&morse_gpio_rx_data_##n,					\
		NULL,								\
		PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,			\
		&morse_gpio_rx_api);

DT_INST_FOREACH_STATUS_OKAY(MORSE_GPIO_TX_INIT)
