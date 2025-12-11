/*
 * Copyright (c), 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18im704_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_sc18im, CONFIG_GPIO_LOG_LEVEL);

#include "i2c/i2c_sc18im704.h"

#define GPIO_SC18IM_MAX_PINS		8

/* After reset the GPIO config registers are 0x55 */
#define GPIO_SC18IM_DEFAULT_CONF	0x55

#define GPIO_SC18IM_CONF_INPUT		0x01
#define GPIO_SC18IM_CONF_PUSH_PULL	0x02
#define GPIO_SC18IM_CONF_OPEN_DRAIN	0x03
#define GPIO_SC18IM_CONF_MASK		0x03

struct gpio_sc18im_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	const struct device *bridge;
};

struct gpio_sc18im_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	uint8_t conf1;
	uint8_t conf2;
	uint8_t output_state;
};

static int gpio_sc18im_port_set_raw(const struct device *port,
				    uint8_t mask, uint8_t value, uint8_t toggle)
{
	const struct gpio_sc18im_config *cfg = port->config;
	struct gpio_sc18im_data *data = port->data;
	uint8_t buf[] = {
		SC18IM704_CMD_WRITE_GPIO,
		data->output_state,
		SC18IM704_CMD_STOP,
	};
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	buf[1] &= ~mask;
	buf[1] |= (value & mask);
	buf[1] ^= toggle;

	ret = sc18im704_transfer(cfg->bridge, buf, sizeof(buf), NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to write GPIO state (%d)", ret);
		return ret;
	}

	data->output_state = buf[1];

	return 0;
}

static int gpio_sc18im_pin_configure(const struct device *port, gpio_pin_t pin,
				     gpio_flags_t flags)
{
	const struct gpio_sc18im_config *cfg = port->config;
	struct gpio_sc18im_data *data = port->data;
	uint8_t pin_conf;
	int ret;
	uint8_t buf[] = {
		SC18IM704_CMD_WRITE_REG,
		0x00,
		0x00,
		SC18IM704_CMD_STOP,
	};

	if (pin >= GPIO_SC18IM_MAX_PINS) {
		return -EINVAL;
	}

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INPUT) {
		pin_conf = GPIO_SC18IM_CONF_INPUT;
	} else if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				pin_conf = GPIO_SC18IM_CONF_OPEN_DRAIN;
			} else {
				/* Open-drain is the only supported single-ended mode */
				return -ENOTSUP;
			}
		} else {
			/* Default to push/pull */
			pin_conf = GPIO_SC18IM_CONF_PUSH_PULL;
		}
	} else {
		/* Neither input nor output mode is selected */
		return -ENOTSUP;
	}

	ret = sc18im704_claim(cfg->bridge);
	if (ret < 0) {
		LOG_ERR("Failed to claim bridge (%d)", ret);
		return ret;
	}

	if (pin < 4) {
		/* Shift the config to the pin offset */
		data->conf1 &= ~(GPIO_SC18IM_CONF_MASK << (pin * 2));
		data->conf1 |= pin_conf << (pin * 2);

		buf[1] = SC18IM704_REG_GPIO_CONF1;
		buf[2] = data->conf1;
	} else {
		/* Shift the config to the pin offset */
		data->conf2 &= ~(GPIO_SC18IM_CONF_MASK << ((pin - 4) * 2));
		data->conf2 |= pin_conf << ((pin - 4) * 2);

		buf[1] = SC18IM704_REG_GPIO_CONF2;
		buf[2] = data->conf2;
	}

	ret = sc18im704_transfer(cfg->bridge, buf, sizeof(buf), NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO (%d)", ret);
	}

	if (ret == 0 && flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_sc18im_port_set_raw(port, BIT(pin), BIT(pin), 0);
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_sc18im_port_set_raw(port, BIT(pin), 0, 0);
		}
	}

	sc18im704_release(cfg->bridge);

	return ret;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_sc18im_pin_get_config(const struct device *port, gpio_pin_t pin,
				      gpio_flags_t *flags)
{
	struct gpio_sc18im_data *data = port->data;
	uint8_t conf;

	if (pin >= GPIO_SC18IM_MAX_PINS) {
		return -EINVAL;
	}

	if (pin < 4) {
		conf = (data->conf1 >> (2 * pin)) & GPIO_SC18IM_CONF_MASK;
	} else {
		conf = (data->conf2 >> (2 * (pin - 4))) & GPIO_SC18IM_CONF_MASK;
	}

	switch (conf) {
	case GPIO_SC18IM_CONF_PUSH_PULL:
		*flags = GPIO_OUTPUT | GPIO_PUSH_PULL;
		break;
	case GPIO_SC18IM_CONF_OPEN_DRAIN:
		*flags = GPIO_OUTPUT | GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN;
		break;
	case GPIO_SC18IM_CONF_INPUT:
	default:
		*flags = GPIO_INPUT;
		break;
	}

	return 0;
}
#endif

static int gpio_sc18im_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_sc18im_config *cfg = port->config;

	uint8_t buf[] = {
		SC18IM704_CMD_READ_GPIO,
		SC18IM704_CMD_STOP,
	};
	uint8_t data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = sc18im704_transfer(cfg->bridge, buf, sizeof(buf), &data, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read GPIO state (%d)", ret);
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_sc18im_port_set_masked_raw(const struct device *port,
					   gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	return gpio_sc18im_port_set_raw(port, (uint8_t)mask, (uint8_t)value, 0);
}

static int gpio_sc18im_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18im_port_set_raw(port, (uint8_t)pins, (uint8_t)pins, 0);
}

static int gpio_sc18im_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18im_port_set_raw(port, (uint8_t)pins, 0, 0);
}

static int gpio_sc18im_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18im_port_set_raw(port, 0, 0, (uint8_t)pins);
}

static int gpio_sc18im_init(const struct device *dev)
{
	const struct gpio_sc18im_config *cfg = dev->config;

	if (!device_is_ready(cfg->bridge)) {
		LOG_ERR("Parent device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_sc18im_driver_api) = {
	.pin_configure = gpio_sc18im_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_sc18im_pin_get_config,
#endif
	.port_get_raw = gpio_sc18im_port_get_raw,
	.port_set_masked_raw = gpio_sc18im_port_set_masked_raw,
	.port_set_bits_raw = gpio_sc18im_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sc18im_port_clear_bits_raw,
	.port_toggle_bits = gpio_sc18im_port_toggle_bits,
};

#define CHECK_COMPAT(node)									\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node, nxp_sc18im704_i2c), (DEVICE_DT_GET(node)), ())

#define GPIO_SC18IM704_I2C_SIBLING(n)								\
	DT_FOREACH_CHILD_STATUS_OKAY(DT_INST_PARENT(n), CHECK_COMPAT)

#define GPIO_SC18IM704_DEFINE(n)								\
	static const struct gpio_sc18im_config gpio_sc18im_config_##n = {			\
		.common = {									\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),			\
		},										\
		.bridge = GPIO_SC18IM704_I2C_SIBLING(n),					\
	};											\
	static struct gpio_sc18im_data gpio_sc18im_data_##n = {					\
		.conf1 = GPIO_SC18IM_DEFAULT_CONF,						\
		.conf2 = GPIO_SC18IM_DEFAULT_CONF,						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, gpio_sc18im_init, NULL,					\
			      &gpio_sc18im_data_##n, &gpio_sc18im_config_##n,			\
			      POST_KERNEL, CONFIG_GPIO_SC18IM704_INIT_PRIORITY,			\
			      &gpio_sc18im_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SC18IM704_DEFINE);
