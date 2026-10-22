/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_bcm283x_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <rpi_fw.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_rpi_bcm283x, CONFIG_GPIO_LOG_LEVEL);

#define RPI_BCM283X_GPIO_DIR_MSK   (GPIO_INPUT | GPIO_OUTPUT)
#define RPI_BCM283X_GPIO_OFFSET(x) (x + 0x80)

struct rpi_bcm283x_gpio_settings {
	uint32_t gpio;
	uint32_t direction;
	uint32_t polarity;
	uint32_t enable;
	uint32_t bias;
	uint32_t state;
};

struct rpi_bcm283x_gpio_state {
	uint32_t gpio;
	uint32_t state;
};

struct gpio_rpi_bcm283x_config {
	/* gpio_driver_config must be first */
	struct gpio_driver_config common;
	const struct device *fw_dev;
	uint8_t ngpios;
};

struct gpio_rpi_bcm283x_data {
	/* gpio_driver_data must be first */
	struct gpio_driver_data common;
};

static int gpio_rpi_bcm283x_pin_configure(const struct device *dev, gpio_pin_t pin,
					  gpio_flags_t flags)
{
	const struct gpio_rpi_bcm283x_config *cfg = dev->config;
	struct rpi_bcm283x_gpio_settings set, get;
	int ret;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		LOG_ERR("Pull resistors not supported");
		return -ENOTSUP;
	}

	if ((flags & RPI_BCM283X_GPIO_DIR_MSK) == RPI_BCM283X_GPIO_DIR_MSK) {
		LOG_ERR("Simultaneous input and output not supported");
		return -ENOTSUP;
	}

	if (flags == GPIO_DISCONNECTED) {
		LOG_ERR("Disconnected mode not supported");
		return -ENOTSUP;
	}

	get.gpio = RPI_BCM283X_GPIO_OFFSET(pin);
	ret = rpi_fw_transfer(cfg->fw_dev, RPI_FW_TAG_GET_GPIO_CONFIG, &get, sizeof(get));
	if (ret < 0) {
		LOG_ERR("Failed to get GPIO config (%d)", ret);
		return ret;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		set.direction = 1;
		/* Drive the initial level if GPIO_OUTPUT_INIT_* flags present */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			set.state = 1;
		} else {
			set.state = 0;
		}
	} else {
		set.direction = 0;
		set.state = 0;
	}

	set.gpio = RPI_BCM283X_GPIO_OFFSET(pin);
	set.polarity = get.polarity;
	set.enable = 0;
	set.bias = 0;

	ret = rpi_fw_transfer(cfg->fw_dev, RPI_FW_TAG_SET_GPIO_CONFIG, &set, sizeof(set));
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO config (%d)", ret);
		return ret;
	}

	return 0;
}

static int gpio_rpi_bcm283x_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_rpi_bcm283x_config *cfg = dev->config;
	struct rpi_bcm283x_gpio_state get;
	int ret;

	*value = 0;
	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		get.gpio = RPI_BCM283X_GPIO_OFFSET(i);
		get.state = 0;

		ret = rpi_fw_transfer(cfg->fw_dev, RPI_FW_TAG_GET_GPIO_STATE, &get, sizeof(get));
		if (ret < 0) {
			LOG_ERR("Failed to get GPIO %u state (%d)", i, ret);
			return ret;
		}

		if (get.state) {
			*value |= BIT(i);
		}
	}

	return 0;
}

static int gpio_rpi_bcm283x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						gpio_port_value_t value)
{
	const struct gpio_rpi_bcm283x_config *cfg = dev->config;
	struct rpi_bcm283x_gpio_state set;
	int ret;

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		if ((mask & BIT(i)) == 0) {
			continue;
		}

		set.gpio = RPI_BCM283X_GPIO_OFFSET(i);
		set.state = !!(value & BIT(i));

		ret = rpi_fw_transfer(cfg->fw_dev, RPI_FW_TAG_SET_GPIO_STATE, &set, sizeof(set));
		if (ret < 0) {
			LOG_ERR("Failed to set GPIO %u state (%d)", i, ret);
			return ret;
		}
	}

	return 0;
}

static int gpio_rpi_bcm283x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_rpi_bcm283x_port_set_masked_raw(dev, pins, pins);
}

static int gpio_rpi_bcm283x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_rpi_bcm283x_port_set_masked_raw(dev, pins, 0);
}

static int gpio_rpi_bcm283x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_port_value_t cur;
	int ret;

	ret = gpio_rpi_bcm283x_port_get_raw(dev, &cur);
	if (ret < 0) {
		return ret;
	}

	return gpio_rpi_bcm283x_port_set_masked_raw(dev, pins, cur ^ pins);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_rpi_bcm283x_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_rpi_bcm283x_config *cfg = dev->config;
	struct rpi_bcm283x_gpio_settings get;
	uint32_t pin;
	int ret;

	if (map & ~GENMASK(cfg->ngpios - 1, 0)) {
		return -EINVAL;
	}

	while (map) {
		pin = find_lsb_set(map) - 1;
		get.gpio = RPI_BCM283X_GPIO_OFFSET(pin);

		ret = rpi_fw_transfer(cfg->fw_dev, RPI_FW_TAG_GET_GPIO_CONFIG, &get, sizeof(get));
		if (ret < 0) {
			LOG_ERR("Failed to get GPIO config (%d)", ret);
			return ret;
		}

		if (inputs != NULL && get.direction == 0) {
			*inputs |= BIT(pin);
		}

		if (outputs != NULL && get.direction == 1) {
			*outputs |= BIT(pin);
		}

		map &= ~BIT(pin);
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static DEVICE_API(gpio, gpio_rpi_bcm283x_api) = {
	.pin_configure = gpio_rpi_bcm283x_pin_configure,
	.port_get_raw = gpio_rpi_bcm283x_port_get_raw,
	.port_set_masked_raw = gpio_rpi_bcm283x_port_set_masked_raw,
	.port_set_bits_raw = gpio_rpi_bcm283x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rpi_bcm283x_port_clear_bits_raw,
	.port_toggle_bits = gpio_rpi_bcm283x_port_toggle_bits,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_rpi_bcm283x_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

static int gpio_rpi_bcm283x_init(const struct device *dev)
{
	const struct gpio_rpi_bcm283x_config *cfg = dev->config;

	if (!device_is_ready(cfg->fw_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->fw_dev);
		return -ENODEV;
	}

	return 0;
}

BUILD_ASSERT(CONFIG_GPIO_RASPBERRYPI_BCM283X_INIT_PRIORITY >
		     CONFIG_RASPBERRYPI_FIRMWARE_INIT_PRIORITY,
	     "CONFIG_GPIO_RASPBERRYPI_BCM283X_INIT_PRIORITY must be higher than "
	     "CONFIG_RASPBERRYPI_FIRMWARE_INIT_PRIORITY");

#define GPIO_RPI_BCM283X_INIT(n)                                                                   \
	static const struct gpio_rpi_bcm283x_config gpio_rpi_bcm283x_cfg_##n = {                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.fw_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct gpio_rpi_bcm283x_data gpio_rpi_bcm283x_data_##n;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_rpi_bcm283x_init, NULL, &gpio_rpi_bcm283x_data_##n,          \
			      &gpio_rpi_bcm283x_cfg_##n, POST_KERNEL,                              \
			      CONFIG_GPIO_RASPBERRYPI_BCM283X_INIT_PRIORITY,                       \
			      &gpio_rpi_bcm283x_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RPI_BCM283X_INIT)
