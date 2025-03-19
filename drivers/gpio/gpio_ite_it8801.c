/*
 * Copyright (c) 2024 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8801_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_ite_it8801, CONFIG_GPIO_LOG_LEVEL);

struct gpio_it8801_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* IT8801 controller dev */
	const struct device *mfd;
	/* I2C device for the MFD parent */
	const struct i2c_dt_spec i2c_dev;
	/* GPIO input pin status register */
	uint8_t reg_ipsr;
	/* GPIO set output value register */
	uint8_t reg_sovr;
	/* GPIO control register */
	uint8_t reg_gpcr;
	/* GPIO interrupt status register */
	uint8_t reg_gpisr;
	/* GPIO interrupt enable register */
	uint8_t reg_gpier;
	uint8_t pin_mask;
};

struct gpio_it8801_data {
	struct gpio_driver_data common;
	struct it8801_mfd_callback it8801_gpio_callback;
	sys_slist_t callbacks;
};

static int ioex_check_is_not_valid(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_it8801_config *config = dev->config;

	if (BIT(pin) & ~(config->pin_mask)) {
		LOG_ERR("GPIO  port%d-%d is not support", config->reg_ipsr, pin);
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_it8801_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;
	uint8_t reg_gpcr = config->reg_gpcr + pin;
	uint8_t mask = BIT(pin);
	uint8_t control;

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) && ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	if (ioex_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, reg_gpcr, &control);
	if (ret) {
		LOG_ERR("Failed to read control value (ret %d)", ret);
		return ret;
	}

	if (flags == GPIO_DISCONNECTED) {
		control &= ~(IT8801_GPIODIR | IT8801_GPIOPDE | IT8801_GPIOPUE);

		goto write_and_return;
	}

	/* If output, set level before changing type to an output. */
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_sovr, mask,
						     mask);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_sovr, mask, 0);
		}
		if (ret) {
			LOG_ERR("Failed to set output value (ret %d)", ret);
			return ret;
		}
		/* Set output */
		control |= IT8801_GPIODIR;
		/* Select open drain 0:push-pull 1:open-drain */
		if (flags & GPIO_OPEN_DRAIN) {
			control |= IT8801_GPIOIOT_OD;
		} else {
			control &= ~IT8801_GPIOIOT_OD;
		}
	} else {
		/* Set input */
		control &= ~IT8801_GPIODIR;
	}

	/* Handle pullup / pulldown */
	if (flags & GPIO_PULL_UP) {
		control = (control | IT8801_GPIOPUE) & ~IT8801_GPIOPDE;
	} else if (flags & GPIO_PULL_DOWN) {
		control = (control | IT8801_GPIOPDE) & ~IT8801_GPIOPUE;
	} else {
		/* No pull up/down */
		control &= ~(IT8801_GPIOPUE | IT8801_GPIOPDE);
	}

write_and_return:
	/* Set GPIO control */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, reg_gpcr, control);
	if (ret) {
		LOG_ERR("Failed to set control value (ret %d)", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_it8801_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct gpio_it8801_config *config = dev->config;
	gpio_flags_t flags = 0;
	int ret;
	uint8_t reg_gpcr = config->reg_gpcr + pin;
	uint8_t mask = BIT(pin);
	uint8_t control, value;

	if (ioex_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, reg_gpcr, &control);
	if (ret) {
		LOG_ERR("Failed to read control value (ret %d)", ret);
		return ret;
	}

	/* Get GPIO direction */
	if (control & IT8801_GPIODIR) {
		flags |= GPIO_OUTPUT;

		/* Get GPIO type, 0:push-pull 1:open-drain */
		if (control & IT8801_GPIOIOT_OD) {
			flags |= GPIO_OPEN_DRAIN;
		}

		ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_ipsr, &value);
		if (ret) {
			LOG_ERR("Failed to read pin status (ret %d)", ret);
			return ret;
		}

		/* Get GPIO output level */
		if (value & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	} else {
		flags |= GPIO_INPUT;
	}

	/* pullup / pulldown */
	if (control & IT8801_GPIOPUE) {
		flags |= GPIO_PULL_UP;
	} else if (control & IT8801_GPIOPDE) {
		flags |= GPIO_PULL_DOWN;
	}

	*out_flags = flags;

	return 0;
}
#endif

static int gpio_it8801_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;
	uint8_t val;

	/* Get raw bits of GPIO mirror register */
	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_ipsr, &val);
	if (ret) {
		LOG_ERR("Failed to get port mask (ret %d)", ret);
		return ret;
	}

	*value = val;

	return 0;
}

static int gpio_it8801_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_sovr, mask, value);
	if (ret) {
		LOG_ERR("Failed to set port mask (ret %d)", ret);
		return ret;
	}

	return 0;
}

static int gpio_it8801_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;

	/* Set raw bits of GPIO data register */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_sovr, pins, pins);
	if (ret) {
		LOG_ERR("Failed to set bits raw (ret %d)", ret);
		return ret;
	}

	return 0;
}

static int gpio_it8801_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;

	/* Clear raw bits of GPIO data register */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_sovr, pins, 0);
	if (ret) {
		LOG_ERR("Failed to clear bits raw (ret %d)", ret);
		return ret;
	}

	return 0;
}

static int gpio_it8801_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_it8801_config *config = dev->config;
	int ret;
	uint8_t val, new_val;

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_sovr, &val);
	if (ret) {
		return ret;
	}
	/* Toggle raw bits of GPIO data register */
	new_val = val ^ pins;
	if (new_val != val) {
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_sovr, new_val);
		if (ret) {
			LOG_ERR("Failed to write toggle value (ret %d)", ret);
			return ret;
		}
	}

	return 0;
}

static int gpio_it8801_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_it8801_data *data = dev->data;
	int ret;

	ret = gpio_manage_callback(&data->callbacks, callback, set);

	return ret;
}

static void it8801_gpio_alert_handler(const struct device *dev)
{
	const struct gpio_it8801_config *config = dev->config;
	struct gpio_it8801_data *data = dev->data;
	int ret;
	uint8_t isr_val, ier_val;

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_gpisr, &isr_val);
	if (ret) {
		LOG_ERR("Failed to read GPIO interrupt status (ret %d)", ret);
		return;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_gpier, &ier_val);
	if (ret) {
		LOG_ERR("Failed to read GPIO interrupt pin set (ret %d)", ret);
		return;
	}

	if (isr_val & ier_val) {
		/* Clear pending interrupt */
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_gpisr, isr_val);
		if (ret) {
			LOG_ERR("Failed to clear GPIO interrupt (ret %d)", ret);
			return;
		}

		gpio_fire_callbacks(&data->callbacks, dev, isr_val);
	}
}

static int gpio_it8801_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_it8801_config *config = dev->config;
	struct gpio_it8801_data *data = dev->data;
	int ret;
	uint8_t reg_gpcr = config->reg_gpcr + pin;
	uint8_t control;
	uint8_t mask = BIT(pin);

	if (ioex_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	/* Disable irq before configuring it  */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_gpier, mask, 0);
	if (ret) {
		LOG_ERR("Failed to disable irq (ret %d)", ret);
		return ret;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		return ret;
	}

	/* Set input pin */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, reg_gpcr, IT8801_GPIODIR, 0);
	if (ret) {
		LOG_ERR("Failed to set input pin (ret %d)", ret);
		return ret;
	}

	/* Clear trigger type */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, reg_gpcr, GENMASK(4, 3), 0);
	if (ret) {
		LOG_ERR("Failed to clear trigger type (ret %d)", ret);
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, reg_gpcr, &control);
	if (ret) {
		LOG_ERR("Failed to read gpio control (ret %d)", ret);
		return ret;
	}

	if (mode == GPIO_INT_MODE_EDGE) {
		/* Set edge trigger */
		if ((trig & GPIO_INT_TRIG_BOTH) == GPIO_INT_TRIG_BOTH) {
			control |= IT8801_GPIOIOT_INT_FALL | IT8801_GPIOIOT_INT_RISE;
		} else if (trig & GPIO_INT_TRIG_LOW) {
			control |= IT8801_GPIOIOT_INT_FALL;
		} else if (trig & GPIO_INT_TRIG_HIGH) {
			control |= IT8801_GPIOIOT_INT_RISE;
		} else {
			LOG_ERR("Invalid interrupt trigger type %d", trig);
			return -EINVAL;
		}
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		/* Set level trigger */
		if (trig & GPIO_INT_TRIG_LOW) {
			control &= ~IT8801_GPIOPOL;
		} else {
			control |= IT8801_GPIOPOL;
		}
	}

	/* Set control value */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, reg_gpcr, control);
	if (ret) {
		LOG_ERR("Failed to write trigger state (ret %d)", ret);
		return ret;
	}

	/* Clear pending interrupt */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_gpisr, mask, mask);
	if (ret) {
		LOG_ERR("Failed to clear pending interrupt (ret %d)", ret);
		return ret;
	}

	/* Enable GPIO interrupt */
	ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_gpier, mask, mask);
	if (ret) {
		LOG_ERR("Failed to enable interrupt (ret %d)", ret);
		return ret;
	}

	/* Gather GPIO interrupt enable */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, IT8801_REG_GIECR, IT8801_REG_MASK_GGPIOIE);

	/* Register the interrupt of IT8801 MFD callback function */
	data->it8801_gpio_callback.cb = it8801_gpio_alert_handler;
	data->it8801_gpio_callback.dev = dev;
	mfd_it8801_register_interrupt_callback(config->mfd, &data->it8801_gpio_callback);

	return ret;
}

static DEVICE_API(gpio, gpio_it8801_driver_api) = {
	.pin_configure = gpio_it8801_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_it8801_get_config,
#endif
	.port_get_raw = gpio_it8801_port_get_raw,
	.port_set_masked_raw = gpio_it8801_port_set_masked_raw,
	.port_set_bits_raw = gpio_it8801_port_set_bits_raw,
	.port_clear_bits_raw = gpio_it8801_port_clear_bits_raw,
	.port_toggle_bits = gpio_it8801_port_toggle_bits,
	.pin_interrupt_configure = gpio_it8801_pin_interrupt_configure,
	.manage_callback = gpio_it8801_manage_callback,
};

static int gpio_it8801_init(const struct device *dev)
{
	const struct gpio_it8801_config *config = dev->config;

	/* Verify multi-function parent is ready */
	if (!device_is_ready(config->mfd)) {
		LOG_ERR("(gpio)%s is not ready", config->mfd->name);
		return -ENODEV;
	}

	return 0;
}

#define GPIO_IT8801_DEVICE_INST(inst)                                                              \
	static struct gpio_it8801_data gpio_it8801_data_##inst;                                    \
	static const struct gpio_it8801_config gpio_it8801_cfg_##inst = {                          \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)},                \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.i2c_dev = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                  \
		.reg_ipsr = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                      \
		.reg_sovr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                      \
		.reg_gpcr = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                      \
		.reg_gpisr = DT_INST_REG_ADDR_BY_IDX(inst, 3),                                     \
		.reg_gpier = DT_INST_REG_ADDR_BY_IDX(inst, 4),                                     \
		.pin_mask = DT_INST_PROP(inst, pin_mask),                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_it8801_init, NULL, &gpio_it8801_data_##inst,              \
			      &gpio_it8801_cfg_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,      \
			      &gpio_it8801_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_IT8801_DEVICE_INST)
