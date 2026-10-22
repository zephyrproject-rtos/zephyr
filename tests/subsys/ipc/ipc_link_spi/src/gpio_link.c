/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_test_gpio_link

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/spinlock.h>

struct gpio_link_config {
	struct gpio_driver_config common;
};

struct gpio_link_data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
	sys_slist_t callbacks;
	gpio_port_value_t values;
	gpio_port_pins_t rising;
	gpio_port_pins_t falling;
};

static void gpio_link_write(const struct device *dev, gpio_port_pins_t mask,
			    gpio_port_value_t values)
{
	struct gpio_link_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	gpio_port_value_t old = data->values;
	gpio_port_pins_t fired;

	data->values = (old & ~mask) | (values & mask);
	fired = (data->values & ~old & data->rising) | (~data->values & old & data->falling);

	k_spin_unlock(&data->lock, key);

	if (fired != 0U) {
		gpio_fire_callbacks(&data->callbacks, dev, fired);
	}
}

static int gpio_link_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		gpio_link_write(dev, BIT(pin), BIT(pin));
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		gpio_link_write(dev, BIT(pin), 0U);
	}

	return 0;
}

static int gpio_link_port_get_raw(const struct device *dev, gpio_port_value_t *values)
{
	struct gpio_link_data *data = dev->data;

	*values = data->values;

	return 0;
}

static int gpio_link_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t values)
{
	gpio_link_write(dev, mask, values);

	return 0;
}

static int gpio_link_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_link_write(dev, pins, pins);

	return 0;
}

static int gpio_link_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_link_write(dev, pins, 0U);

	return 0;
}

static int gpio_link_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_link_data *data = dev->data;

	gpio_link_write(dev, pins, ~data->values);

	return 0;
}

static int gpio_link_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	struct gpio_link_data *data = dev->data;
	k_spinlock_key_t key;

	if (mode != GPIO_INT_MODE_DISABLED && mode != GPIO_INT_MODE_EDGE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	data->rising &= ~BIT(pin);
	data->falling &= ~BIT(pin);

	if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_HIGH || trig == GPIO_INT_TRIG_BOTH) {
			data->rising |= BIT(pin);
		}

		if (trig == GPIO_INT_TRIG_LOW || trig == GPIO_INT_TRIG_BOTH) {
			data->falling |= BIT(pin);
		}
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_link_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_link_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, gpio_link_api) = {
	.pin_configure = gpio_link_pin_configure,
	.port_get_raw = gpio_link_port_get_raw,
	.port_set_masked_raw = gpio_link_port_set_masked_raw,
	.port_set_bits_raw = gpio_link_port_set_bits_raw,
	.port_clear_bits_raw = gpio_link_port_clear_bits_raw,
	.port_toggle_bits = gpio_link_port_toggle_bits,
	.pin_interrupt_configure = gpio_link_pin_interrupt_configure,
	.manage_callback = gpio_link_manage_callback,
};

#define GPIO_LINK_DEFINE(inst)                                                                     \
	static const struct gpio_link_config gpio_link_config_##inst = {                           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
	};                                                                                         \
                                                                                                   \
	static struct gpio_link_data gpio_link_data_##inst;                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &gpio_link_data_##inst, &gpio_link_config_##inst,  \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_link_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_LINK_DEFINE)
