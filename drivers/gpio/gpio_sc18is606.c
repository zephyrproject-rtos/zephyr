/*
 * Copyright (c), 2025 tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18is606_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <mfd_sc18is606.h>

LOG_MODULE_REGISTER(nxp_sc18is606_gpio, CONFIG_GPIO_LOG_LEVEL);

struct gpio_sc18is606_config {
	struct gpio_driver_config common;
	const struct device *bridge;
};

struct gpio_sc18is606_data {
	struct gpio_driver_data sc18is606_data;

	/* current port state */
	uint8_t output_state;
};

static int gpio_sc18is606_port_set_raw(const struct device *port, uint8_t mask, uint8_t value,
				       uint8_t toggle)
{
	const struct gpio_sc18is606_config *cfg = port->config;
	struct gpio_sc18is606_data *data = port->data;

	uint8_t buf[] = {
		SC18IS606_GPIO_WRITE,
		data->output_state,
	};
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	buf[1] &= ~mask;
	buf[1] |= (value & mask);
	buf[1] ^= toggle;

	ret = nxp_sc18is606_transfer(cfg->bridge, buf, sizeof(buf), NULL, 0, NULL);

	if (ret < 0) {
		LOG_ERR("Failed to write to GPIO (%d)", ret);
		return ret;
	}

	data->output_state = buf[1];

	return 0;
}

static int gpio_sc18is606_pin_configure(const struct device *port, gpio_pin_t pin,
					gpio_flags_t flags)
{
	const struct gpio_sc18is606_config *cfg = port->config;
	uint8_t pin_conf;
	int ret;

	if (pin >= SC18IS606_GPIO_MAX_PINS) {
		return -EINVAL;
	}

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INPUT) {
		pin_conf = SC18IS606_GPIO_CONF_INPUT;
	} else if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				pin_conf = SC18IS606_GPIO_CONF_OPEN_DRAIN;
			} else {
				return -ENOTSUP;
			}
		} else {
			pin_conf = SC18IS606_GPIO_CONF_PUSH_PULL;
		}
	} else {
		/* Impossible option */
		return -ENOTSUP;
	}

	ret = nxp_sc18is606_set_pin_mode(cfg->bridge, pin, true, pin_conf);
	if (ret < 0) {
		LOG_ERR("Failed to set pin mode (%d)", ret);
	}

	if (ret == 0 && flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_sc18is606_port_set_raw(port, BIT(pin), BIT(pin), 0);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_sc18is606_port_set_raw(port, BIT(pin), 0, 0);
		}
	}

	return ret;
}

static int gpio_sc18is606_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_sc18is606_config *cfg = port->config;

	uint8_t buf[] = {
		SC18IS606_GPIO_READ,
	};
	uint8_t data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = nxp_sc18is606_transfer(cfg->bridge, buf, sizeof(buf), &data, 1, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to read GPIO state (%d)", ret);
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_sc18is606_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	return gpio_sc18is606_port_set_raw(port, (uint8_t)mask, (uint8_t)value, 0);
}

static int gpio_sc18is606_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is606_port_set_raw(port, (uint8_t)pins, (uint8_t)pins, 0);
}

static int gpio_sc18is606_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is606_port_set_raw(port, (uint8_t)pins, 0, 0);
}
static int gpio_sc18is606_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is606_port_set_raw(port, 0, 0, (uint8_t)pins);
}

static int gpio_sc18is606_init(const struct device *dev)
{
	const struct gpio_sc18is606_config *cfg = dev->config;

	if (!device_is_ready(cfg->bridge)) {
		LOG_ERR("Parent device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_sc18is606_driver_api) = {
	.pin_configure = gpio_sc18is606_pin_configure,
	.port_get_raw = gpio_sc18is606_port_get_raw,
	.port_set_masked_raw = gpio_sc18is606_port_set_masked_raw,
	.port_set_bits_raw = gpio_sc18is606_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sc18is606_port_clear_bits_raw,
	.port_toggle_bits = gpio_sc18is606_port_toggle_bits,
};

#define GPIO_SC18IS606_DEFINE(inst)                                                                \
	static const struct gpio_sc18is606_config gpio_sc18is606_config##inst = {                  \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.bridge = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	static struct gpio_sc18is606_data gpio_sc18is606_data##inst = {0};                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_sc18is606_init, NULL, &gpio_sc18is606_data##inst,         \
			      &gpio_sc18is606_config##inst, POST_KERNEL,                           \
			      CONFIG_GPIO_SC18IS606_INIT_PRIORITY, &gpio_sc18is606_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SC18IS606_DEFINE);
