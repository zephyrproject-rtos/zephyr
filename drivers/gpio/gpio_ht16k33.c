/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT holtek_ht16k33_keyscan

/**
 * @file
 * @brief GPIO driver for the HT16K33 I2C LED driver with keyscan
 */

#include <drivers/gpio.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_ht16k33);

#include <drivers/led/ht16k33.h>

#include "gpio_utils.h"

/* HT16K33 size definitions */
#define HT16K33_KEYSCAN_ROWS 3

struct gpio_ht16k33_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	char *parent_dev_name;
	uint8_t keyscan_idx;
};

struct gpio_ht16k33_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct device *parent;
	sys_slist_t callbacks;
};

static int gpio_ht16k33_cfg(struct device *dev,
			    gpio_pin_t pin,
			    gpio_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);

	/* Keyscan is input-only */
	if (((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED)
	    || ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_ht16k33_port_get_raw(struct device *port,
				     gpio_port_value_t *value)
{
	ARG_UNUSED(port);
	ARG_UNUSED(value);

	/* Keyscan only supports interrupt mode */
	return -ENOTSUP;
}

static int gpio_ht16k33_port_set_masked_raw(struct device *port,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	ARG_UNUSED(port);
	ARG_UNUSED(mask);
	ARG_UNUSED(value);

	/* Keyscan is input-only */
	return -ENOTSUP;
}

static int gpio_ht16k33_port_set_bits_raw(struct device *port,
					  gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Keyscan is input-only */
	return -ENOTSUP;
}

static int gpio_ht16k33_port_clear_bits_raw(struct device *port,
					    gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Keyscan is input-only */
	return -ENOTSUP;
}

static int gpio_ht16k33_port_toggle_bits(struct device *port,
					 gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Keyscan is input-only */
	return -ENOTSUP;
}

static int gpio_ht16k33_pin_interrupt_configure(struct device *port,
						gpio_pin_t pin,
						enum gpio_int_mode int_mode,
						enum gpio_int_trig int_trig)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pin);
	ARG_UNUSED(int_mode);
	ARG_UNUSED(int_trig);

	/* Interrupts are always enabled */
	return 0;
}

void ht16k33_process_keyscan_row_data(struct device *dev,
				      uint32_t keys)
{
	struct gpio_ht16k33_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, keys);
}

static int gpio_ht16k33_manage_callback(struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct gpio_ht16k33_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_ht16k33_get_pending_int(struct device *dev)
{
	struct gpio_ht16k33_data *data = dev->data;

	return ht16k33_get_pending_int(data->parent);
}

static int gpio_ht16k33_init(struct device *dev)
{
	const struct gpio_ht16k33_cfg *config = dev->config;
	struct gpio_ht16k33_data *data = dev->data;

	if (config->keyscan_idx >= HT16K33_KEYSCAN_ROWS) {
		LOG_ERR("HT16K33 keyscan index out of bounds (%d)",
			config->keyscan_idx);
		return -EINVAL;
	}

	/* Establish reference to parent and vice versa */
	data->parent = device_get_binding(config->parent_dev_name);
	if (!data->parent) {
		LOG_ERR("HT16K33 parent device '%s' not found",
			config->parent_dev_name);
		return -EINVAL;
	}

	return ht16k33_register_keyscan_device(data->parent, dev,
					       config->keyscan_idx);
}

static const struct gpio_driver_api gpio_ht16k33_api = {
	.pin_configure = gpio_ht16k33_cfg,
	.port_get_raw = gpio_ht16k33_port_get_raw,
	.port_set_masked_raw = gpio_ht16k33_port_set_masked_raw,
	.port_set_bits_raw = gpio_ht16k33_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ht16k33_port_clear_bits_raw,
	.port_toggle_bits = gpio_ht16k33_port_toggle_bits,
	.pin_interrupt_configure = gpio_ht16k33_pin_interrupt_configure,
	.manage_callback = gpio_ht16k33_manage_callback,
	.get_pending_int = gpio_ht16k33_get_pending_int,
};

#define GPIO_HT16K33_DEVICE(id)						\
	static const struct gpio_ht16k33_cfg gpio_ht16k33_##id##_cfg = {\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(13),		\
		},							\
		.parent_dev_name =					\
			DT_INST_BUS_LABEL(id),	\
		.keyscan_idx     =					\
			DT_INST_REG_ADDR(id),	\
	};								\
									\
	static struct gpio_ht16k33_data gpio_ht16k33_##id##_data;	\
									\
	DEVICE_AND_API_INIT(gpio_ht16k33_##id,				\
			    DT_INST_LABEL(id),	\
			    &gpio_ht16k33_init,				\
			    &gpio_ht16k33_##id##_data,			\
			    &gpio_ht16k33_##id##_cfg, POST_KERNEL,	\
			    CONFIG_GPIO_HT16K33_INIT_PRIORITY,		\
			    &gpio_ht16k33_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_HT16K33_DEVICE)
