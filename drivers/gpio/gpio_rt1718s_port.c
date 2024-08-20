/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT richtek_rt1718s_gpio_port

/**
 * @file Driver for RS1718S TCPC chip GPIOs.
 */

#include "gpio_rt1718s.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(gpio_rt1718s_port, CONFIG_GPIO_LOG_LEVEL);

/* Driver config */
struct gpio_rt1718s_port_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* RT1718S chip device */
	const struct device *rt1718s_dev;
};

/* Driver data */
struct gpio_rt1718s_port_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* GPIO callback list */
	sys_slist_t cb_list_gpio;
	/* lock GPIO registers access */
	struct k_sem lock;
};

/* GPIO api functions */
static int gpio_rt1718s_pin_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	uint8_t new_reg = 0;
	int ret = 0;

	/* Don't support simultaneous in/out mode */
	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	/* Don't support "open source" mode */
	if ((flags & GPIO_SINGLE_ENDED) && !(flags & GPIO_LINE_OPEN_DRAIN)) {
		return -ENOTSUP;
	}

	/* RT1718S has 3 GPIOs so check range */
	if (pin >= RT1718S_GPIO_NUM) {
		return -EINVAL;
	}

	/* Configure pin as input. */
	if (flags & GPIO_INPUT) {
		/* Do not set RT1718S_REG_GPIO_CTRL_OE bit for input */
		/* Set pull-high/low input */
		if (flags & GPIO_PULL_UP) {
			new_reg |= RT1718S_REG_GPIO_CTRL_PU;
		}
		if (flags & GPIO_PULL_DOWN) {
			new_reg |= RT1718S_REG_GPIO_CTRL_PD;
		}
	} else if (flags & GPIO_OUTPUT) {
		/* Set GPIO as output */
		new_reg |= RT1718S_REG_GPIO_CTRL_OE;

		/* Set push-pull or open-drain */
		if (!(flags & GPIO_SINGLE_ENDED)) {
			new_reg |= RT1718S_REG_GPIO_CTRL_OD_N;
		}

		/* Set init state */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			new_reg |= RT1718S_REG_GPIO_CTRL_O;
		}
	}

	k_sem_take(&data->lock, K_FOREVER);
	ret = rt1718s_reg_write_byte(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin), new_reg);
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	uint8_t reg;
	int ret;

	ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_RT_ST8, &reg);
	*value = reg & (RT1718S_REG_RT_ST8_GPIO1_I | RT1718S_REG_RT_ST8_GPIO2_I |
			RT1718S_REG_RT_ST8_GPIO3_I);

	return ret;
}

static int gpio_rt1718s_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	uint8_t new_reg, reg;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	for (int pin = 0; pin < RT1718S_GPIO_NUM; pin++) {
		if (mask & BIT(pin)) {
			ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						    &reg);
			if (ret < 0) {
				break;
			}

			if (value & BIT(pin)) {
				new_reg = reg | RT1718S_REG_GPIO_CTRL_O;
			} else {
				new_reg = reg & ~RT1718S_REG_GPIO_CTRL_O;
			}
			ret = rt1718s_reg_update(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						 reg, new_reg);
		}
	}

	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	uint8_t new_reg, reg;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	for (int pin = 0; pin < RT1718S_GPIO_NUM; pin++) {
		if (mask & BIT(pin)) {
			ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						    &reg);
			if (ret < 0) {
				break;
			}
			new_reg = reg | RT1718S_REG_GPIO_CTRL_O;
			ret = rt1718s_reg_update(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						 reg, new_reg);
		}
	}

	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	uint8_t new_reg, reg;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	for (int pin = 0; pin < RT1718S_GPIO_NUM; pin++) {
		if (mask & BIT(pin)) {
			ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						    &reg);
			if (ret < 0) {
				break;
			}
			new_reg = reg & ~RT1718S_REG_GPIO_CTRL_O;
			ret = rt1718s_reg_update(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						 reg, new_reg);
		}
	}

	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	uint8_t new_reg, reg;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	for (int pin = 0; pin < RT1718S_GPIO_NUM; pin++) {
		if (mask & BIT(pin)) {
			ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						    &reg);
			if (ret < 0) {
				break;
			}
			new_reg = reg ^ RT1718S_REG_GPIO_CTRL_O;
			ret = rt1718s_reg_update(config->rt1718s_dev, RT1718S_REG_GPIO_CTRL(pin),
						 reg, new_reg);
		}
	}

	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;
	struct rt1718s_data *const data_rt1718s = config->rt1718s_dev->data;
	uint8_t reg_int8, reg_mask8, new_reg_mask8 = 0;
	uint8_t mask_rise = BIT(pin), mask_fall = BIT(4 + pin);
	uint16_t alert_mask;
	int ret;

	/* Check passed arguments */
	if (mode == GPIO_INT_MODE_LEVEL || pin >= RT1718S_GPIO_NUM) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	k_sem_take(&data_rt1718s->lock_tcpci, K_FOREVER);

	ret = rt1718s_reg_read_byte(config->rt1718s_dev, RT1718S_REG_RT_MASK8, &reg_mask8);
	if (ret < 0) {
		goto done;
	}

	/* Disable GPIO interrupt */
	if (mode == GPIO_INT_MODE_DISABLED) {
		new_reg_mask8 = reg_mask8 & ~(mask_rise | mask_fall);
	} else if (mode == GPIO_INT_MODE_EDGE) {
		switch (trig) {
		case GPIO_INT_TRIG_BOTH:
			new_reg_mask8 = reg_mask8 | mask_rise | mask_fall;
			break;
		case GPIO_INT_TRIG_HIGH:
			new_reg_mask8 = (reg_mask8 | mask_rise) & ~mask_fall;
			break;
		case GPIO_INT_TRIG_LOW:
			new_reg_mask8 = (reg_mask8 | mask_fall) & ~mask_rise;
			break;
		default:
			ret = -EINVAL;
			goto done;
		}

		ret = rt1718s_reg_burst_read(config->rt1718s_dev, RT1718S_REG_ALERT_MASK,
					     (uint8_t *)&alert_mask, sizeof(alert_mask));
		if (ret) {
			goto done;
		}

		/* Enable Vendor Defined Alert for GPIO interrupts */
		if (!(alert_mask & RT1718S_REG_ALERT_MASK_VENDOR_DEFINED_ALERT)) {
			alert_mask |= RT1718S_REG_ALERT_MASK_VENDOR_DEFINED_ALERT;
			ret = rt1718s_reg_burst_write(config->rt1718s_dev, RT1718S_REG_ALERT_MASK,
						      (uint8_t *)&alert_mask, sizeof(alert_mask));

			if (ret) {
				goto done;
			}
		}

		/* Clear pending interrupts, which were trigger before enabling the pin
		 * interrupt by user.
		 */
		reg_int8 = mask_rise | mask_fall;
		rt1718s_reg_write_byte(config->rt1718s_dev, RT1718S_REG_RT_INT8, reg_int8);
	}

	/* MASK8 handles 3 GPIOs interrupts, both edges */
	ret = rt1718s_reg_update(config->rt1718s_dev, RT1718S_REG_RT_MASK8, reg_mask8,
				 new_reg_mask8);

done:
	k_sem_give(&data_rt1718s->lock_tcpci);
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_rt1718s_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_rt1718s_port_data *const data = dev->data;

	return gpio_manage_callback(&data->cb_list_gpio, callback, set);
}

void rt1718s_gpio_alert_handler(const struct device *dev)
{
	const struct rt1718s_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data_port = config->gpio_port_dev->data;
	uint8_t reg_int8, reg_mask8;

	k_sem_take(&data_port->lock, K_FOREVER);

	/* Get mask and state of GPIO interrupts */
	if (rt1718s_reg_read_byte(dev, RT1718S_REG_RT_INT8, &reg_int8) ||
	    rt1718s_reg_read_byte(dev, RT1718S_REG_RT_MASK8, &reg_mask8)) {
		k_sem_give(&data_port->lock);
		LOG_ERR("i2c access failed");
		return;
	}

	reg_int8 &= reg_mask8;
	/* Clear the interrupts */
	if (reg_int8) {
		if (rt1718s_reg_write_byte(dev, RT1718S_REG_RT_INT8, reg_int8)) {
			k_sem_give(&data_port->lock);
			LOG_ERR("i2c access failed");
			return;
		}
	}

	k_sem_give(&data_port->lock);

	if (reg_int8 & RT1718S_GPIO_INT_MASK) {
		/* Call the GPIO callbacks for rising *or* falling edge */
		gpio_fire_callbacks(&data_port->cb_list_gpio, config->gpio_port_dev,
				    (reg_int8 & 0x7) | ((reg_int8 >> 4) & 0x7));
	}
}

static const struct gpio_driver_api gpio_rt1718s_driver = {
	.pin_configure = gpio_rt1718s_pin_config,
	.port_get_raw = gpio_rt1718s_port_get_raw,
	.port_set_masked_raw = gpio_rt1718s_port_set_masked_raw,
	.port_set_bits_raw = gpio_rt1718s_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rt1718s_port_clear_bits_raw,
	.port_toggle_bits = gpio_rt1718s_port_toggle_bits,
	.pin_interrupt_configure = gpio_rt1718s_pin_interrupt_configure,
	.manage_callback = gpio_rt1718s_manage_callback,
};

static int gpio_rt1718s_port_init(const struct device *dev)
{
	const struct gpio_rt1718s_port_config *const config = dev->config;
	struct gpio_rt1718s_port_data *const data = dev->data;

	if (!device_is_ready(config->rt1718s_dev)) {
		LOG_ERR("%s is not ready", config->rt1718s_dev->name);
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}

/* RT1718S GPIO port driver must be initialized after RT1718S chip driver */
BUILD_ASSERT(CONFIG_GPIO_RT1718S_PORT_INIT_PRIORITY > CONFIG_RT1718S_INIT_PRIORITY);

#define GPIO_RT1718S_PORT_DEVICE_INSTANCE(inst)                                                    \
	static const struct gpio_rt1718s_port_config gpio_rt1718s_port_cfg_##inst = {              \
		.common = {.port_pin_mask = 0x7},                                                  \
		.rt1718s_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                \
	};                                                                                         \
	static struct gpio_rt1718s_port_data gpio_rt1718s_port_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst, gpio_rt1718s_port_init, NULL, &gpio_rt1718s_port_data_##inst,  \
			      &gpio_rt1718s_port_cfg_##inst, POST_KERNEL,                          \
			      CONFIG_GPIO_RT1718S_PORT_INIT_PRIORITY, &gpio_rt1718s_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RT1718S_PORT_DEVICE_INSTANCE)
