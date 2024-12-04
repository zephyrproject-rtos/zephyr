/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad559x_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/ad559x.h>

#define AD559X_GPIO_RD_POINTER 0x60

struct gpio_ad559x_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *mfd_dev;
};

struct gpio_ad559x_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	uint8_t gpio_val;
	uint8_t gpio_out;
	uint8_t gpio_in;
	uint8_t gpio_pull_down;
};

static int gpio_ad559x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ad559x_config *config = dev->config;
	struct gpio_ad559x_data *drv_data = dev->data;
	uint16_t data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (mfd_ad559x_has_pointer_byte_map(config->mfd_dev)) {
		ret = mfd_ad559x_read_reg(config->mfd_dev, AD559X_GPIO_RD_POINTER, 0, &data);
		/* LSB contains port information. Clear the MSB. */
		data &= BIT_MASK(AD559X_PIN_MAX);
	} else {
		ret = mfd_ad559x_read_reg(config->mfd_dev, AD559X_REG_GPIO_INPUT_EN,
					  drv_data->gpio_in, &data);
	}

	if (ret < 0) {
		return ret;
	}

	*value = (uint32_t)data;

	return 0;
}

static int gpio_ad559x_port_set_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	struct gpio_ad559x_data *data = dev->data;
	const struct gpio_ad559x_config *config = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	data->gpio_val |= (uint8_t)pins;

	return mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_GPIO_SET, data->gpio_val);
}

static int gpio_ad559x_port_clear_bits_raw(const struct device *dev,
					    gpio_port_pins_t pins)
{
	struct gpio_ad559x_data *data = dev->data;
	const struct gpio_ad559x_config *config = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	data->gpio_val &= ~(uint8_t)pins;

	return mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_GPIO_SET, data->gpio_val);
}

static inline int gpio_ad559x_configure(const struct device *dev,
					 gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_ad559x_data *data = dev->data;
	const struct gpio_ad559x_config *config = dev->config;
	uint8_t val;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= AD559X_PIN_MAX) {
		return -EINVAL;
	}

	val = BIT(pin);
	if ((flags & GPIO_OUTPUT) != 0U) {
		data->gpio_in &= ~val;
		data->gpio_out |= val;

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			ret = gpio_ad559x_port_set_bits_raw(
				dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			ret = gpio_ad559x_port_clear_bits_raw(
				dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		}

		ret = mfd_ad559x_write_reg(config->mfd_dev,
					   AD559X_REG_GPIO_OUTPUT_EN, data->gpio_out);
		if (ret < 0) {
			return ret;
		}

		ret = mfd_ad559x_write_reg(config->mfd_dev,
					   AD559X_REG_GPIO_INPUT_EN, data->gpio_in);
	} else if ((flags & GPIO_INPUT) != 0U) {
		data->gpio_in |= val;
		data->gpio_out &= ~val;

		if ((flags & GPIO_PULL_DOWN) != 0U) {
			data->gpio_pull_down |= val;

			ret = mfd_ad559x_write_reg(config->mfd_dev,
						   AD559X_REG_GPIO_PULLDOWN,
						   data->gpio_pull_down);
			if (ret < 0) {
				return ret;
			}
		} else if ((flags & GPIO_PULL_UP) != 0U) {
			return -ENOTSUP;
		}

		ret = mfd_ad559x_write_reg(config->mfd_dev,
					   AD559X_REG_GPIO_OUTPUT_EN, data->gpio_out);
		if (ret < 0) {
			return ret;
		}

		ret = mfd_ad559x_write_reg(config->mfd_dev,
					   AD559X_REG_GPIO_INPUT_EN, data->gpio_in);
	} else {
		return -ENOTSUP;
	}

	return ret;
}

static int gpio_ad559x_port_set_masked_raw(const struct device *dev,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mask);
	ARG_UNUSED(value);

	return -ENOTSUP;
}

static int gpio_ad559x_port_toggle_bits(const struct device *dev,
					 gpio_port_pins_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	return -ENOTSUP;
}

static int gpio_ad559x_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

static DEVICE_API(gpio, gpio_ad559x_api) = {
	.pin_configure = gpio_ad559x_configure,
	.port_get_raw = gpio_ad559x_port_get_raw,
	.port_set_masked_raw = gpio_ad559x_port_set_masked_raw,
	.port_set_bits_raw = gpio_ad559x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ad559x_port_clear_bits_raw,
	.port_toggle_bits = gpio_ad559x_port_toggle_bits,
	.pin_interrupt_configure = gpio_ad559x_pin_interrupt_configure,
};

static int gpio_ad559x_init(const struct device *dev)
{
	const struct gpio_ad559x_config *config = dev->config;

	if (!device_is_ready(config->mfd_dev)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_AD559X_DEFINE(inst)							\
	static const struct gpio_ad559x_config gpio_ad559x_config##inst = {		\
		.common = {								\
			.port_pin_mask =						\
			GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),				\
		},									\
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
	};										\
											\
	static struct gpio_ad559x_data gpio_ad559x_data##inst;				\
											\
	DEVICE_DT_INST_DEFINE(inst, gpio_ad559x_init, NULL,				\
			      &gpio_ad559x_data##inst, &gpio_ad559x_config##inst,	\
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,			\
			      &gpio_ad559x_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AD559X_DEFINE)
