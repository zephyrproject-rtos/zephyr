/*
 * SPDX-FileCopyrightText: Copyright tinyvision.ai
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/mfd/mfd_sc18is60x.h>

LOG_MODULE_REGISTER(nxp_sc18is60x_gpio, CONFIG_GPIO_LOG_LEVEL);

#define SC18IS60X_GPIO_WRITE  0xF4
#define SC18IS60X_GPIO_READ   0xF5
#define SC18IS60X_GPIO_ENABLE 0xF6
#define SC18IS60X_GPIO_CONF   0xF7

#define SC18IS60X_GPIO_CONF_INPUT      0x00
#define SC18IS60X_GPIO_CONF_PUSH_PULL  0x01
#define SC18IS60X_GPIO_CONF_OPEN_DRAIN 0x03
#define SC18IS60X_GPIO_CONF_MASK       0x03

struct gpio_sc18is60x_config {
	struct gpio_driver_config common;
	const struct device *bridge;
};

struct gpio_sc18is60x_data {
	struct gpio_driver_data common;

	uint8_t output_state;
	uint8_t conf;
	uint8_t enable;
};

static int gpio_sc18is60x_port_set_raw_unlocked(const struct device *port, uint8_t mask,
					uint8_t value, uint8_t toggle)
{
	const struct gpio_sc18is60x_config *cfg = port->config;
	struct gpio_sc18is60x_data *data = port->data;
	uint8_t new_state;
	uint8_t buf[] = {
		SC18IS60X_GPIO_WRITE,
		0x00,
	};
	int ret;

	new_state = data->output_state;
	new_state &= ~mask;
	new_state |= (value & mask);
	new_state ^= toggle;
	buf[1] = new_state;

	ret = nxp_sc18is60x_transfer_unlocked(cfg->bridge, buf, sizeof(buf), NULL, 0);
	if (ret != 0) {
		return ret;
	}

	data->output_state = new_state;
	return 0;
}

static int gpio_sc18is60x_port_set_raw(const struct device *port, uint8_t mask, uint8_t value,
					uint8_t toggle)
{
	const struct gpio_sc18is60x_config *cfg = port->config;
	int ret;

	ret = nxp_sc18is60x_lock(cfg->bridge);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_sc18is60x_port_set_raw_unlocked(port, mask, value, toggle);
	nxp_sc18is60x_unlock(cfg->bridge);
	return ret;
}

static int gpio_sc18is60x_pin_configure(const struct device *port, gpio_pin_t pin,
					gpio_flags_t flags)
{
	const struct gpio_sc18is60x_config *cfg = port->config;
	struct gpio_sc18is60x_data *data = port->data;
	uint8_t pin_conf;
	uint8_t new_enable;
	uint8_t new_conf;
	int ret;

	uint8_t enable_buf[] = {
		SC18IS60X_GPIO_ENABLE,
		0x00,
	};

	uint8_t conf_buf[] = {
		SC18IS60X_GPIO_CONF,
		0x00,
	};

	if ((BIT(pin) & cfg->common.port_pin_mask) == 0U) {
		return -EINVAL;
	}

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INPUT) {
		pin_conf = SC18IS60X_GPIO_CONF_INPUT;
	} else if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			pin_conf = SC18IS60X_GPIO_CONF_OPEN_DRAIN;
		} else {
			pin_conf = SC18IS60X_GPIO_CONF_PUSH_PULL;
		}
	} else {
		return -ENOTSUP;
	}

	new_enable = data->enable | BIT(pin);
	enable_buf[1] = new_enable;

	new_conf = data->conf;
	new_conf &= ~(SC18IS60X_GPIO_CONF_MASK << (pin * 2));
	new_conf |= (pin_conf & SC18IS60X_GPIO_CONF_MASK) << (pin * 2);
	conf_buf[1] = new_conf;

	ret = nxp_sc18is60x_lock(cfg->bridge);
	if (ret != 0) {
		return ret;
	}

	ret = nxp_sc18is60x_transfer_unlocked(cfg->bridge, enable_buf, sizeof(enable_buf),
		NULL, 0);
	if (ret != 0) {
		goto out;
	}

	data->enable = new_enable;

	ret = nxp_sc18is60x_transfer_unlocked(cfg->bridge, conf_buf, sizeof(conf_buf), NULL, 0);
	if (ret != 0) {
		goto out;
	}

	data->conf = new_conf;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ret = gpio_sc18is60x_port_set_raw_unlocked(port, BIT(pin), BIT(pin), 0);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ret = gpio_sc18is60x_port_set_raw_unlocked(port, BIT(pin), 0, 0);
		}
	}

out:
	nxp_sc18is60x_unlock(cfg->bridge);
	return ret;
}

static int gpio_sc18is60x_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_sc18is60x_config *cfg = port->config;

	uint8_t buf[] = {
		SC18IS60X_GPIO_READ,
	};
	uint8_t data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = nxp_sc18is60x_transfer(cfg->bridge, buf, sizeof(buf), &data, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read GPIO state (%d)", ret);
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_sc18is60x_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	return gpio_sc18is60x_port_set_raw(port, (uint8_t)mask, (uint8_t)value, 0);
}

static int gpio_sc18is60x_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is60x_port_set_raw(port, (uint8_t)pins, (uint8_t)pins, 0);
}

static int gpio_sc18is60x_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is60x_port_set_raw(port, (uint8_t)pins, 0, 0);
}

static int gpio_sc18is60x_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	return gpio_sc18is60x_port_set_raw(port, 0, 0, (uint8_t)pins);
}

static int gpio_sc18is60x_init(const struct device *dev)
{
	const struct gpio_sc18is60x_config *cfg = dev->config;

	if (!device_is_ready(cfg->bridge)) {
		LOG_ERR("Parent device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_sc18is60x_driver_api) = {
	.pin_configure = gpio_sc18is60x_pin_configure,
	.port_get_raw = gpio_sc18is60x_port_get_raw,
	.port_set_masked_raw = gpio_sc18is60x_port_set_masked_raw,
	.port_set_bits_raw = gpio_sc18is60x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sc18is60x_port_clear_bits_raw,
	.port_toggle_bits = gpio_sc18is60x_port_toggle_bits,
};

#define GPIO_SC18IS60X_DEFINE(inst, prefix)                                                        \
	static const struct gpio_sc18is60x_config prefix##_cfg_##inst = {                          \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.bridge = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
	static struct gpio_sc18is60x_data prefix##_data_##inst = {                                 \
		.conf = 0x00,                                                                      \
		.enable = 0x00,                                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_sc18is60x_init, NULL, &prefix##_data_##inst,              \
			      &prefix##_cfg_##inst, POST_KERNEL,                                   \
			      CONFIG_GPIO_SC18IS60X_INIT_PRIORITY, &gpio_sc18is60x_driver_api);

#define DT_DRV_COMPAT nxp_sc18is60x_gpio
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_SC18IS60X_DEFINE, sc18is60x_gpio)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_sc18is606_gpio
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_SC18IS60X_DEFINE, sc18is606_gpio)
