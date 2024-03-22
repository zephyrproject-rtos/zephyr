/*
 * Copyright (c) 2023 Martin Kiepfer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp192_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mfd/axp192.h>

LOG_MODULE_REGISTER(gpio_axp192, CONFIG_GPIO_LOG_LEVEL);

struct gpio_axp192_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	const struct device *mfd;
	uint32_t ngpios;
};

struct gpio_axp192_data {
	struct gpio_driver_data common;
	struct k_mutex mutex;
	sys_slist_t cb_list_gpio;
};

static int gpio_axp192_port_get_raw(const struct device *dev, uint32_t *value)
{
	int ret;
	uint8_t port_val;
	const struct gpio_axp192_config *config = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = mfd_axp192_gpio_read_port(config->mfd, &port_val);
	if (ret == 0) {
		*value = port_val;
	}

	return ret;
}

static int gpio_axp192_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	int ret;
	const struct gpio_axp192_config *config = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = mfd_axp192_gpio_write_port(config->mfd, value, mask);

	return ret;
}

static int gpio_axp192_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_axp192_port_set_masked_raw(dev, pins, pins);
}

static int gpio_axp192_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_axp192_port_set_masked_raw(dev, pins, 0);
}

static int gpio_axp192_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_axp192_config *config = dev->config;
	int ret;
	enum axp192_gpio_func func;

	if (pin >= config->ngpios) {
		LOG_ERR("Invalid gpio pin (%d)", pin);
		return -EINVAL;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Configure pin */
	LOG_DBG("Pin: %d / flags=0x%x", pin, flags);
	if ((flags & GPIO_OUTPUT) != 0) {

		/* Initialize output function */
		func = AXP192_GPIO_FUNC_OUTPUT_LOW;
		if ((flags & GPIO_OPEN_DRAIN) != 0) {
			func = AXP192_GPIO_FUNC_OUTPUT_OD;
		}
		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, pin, func);
		if (ret != 0) {
			return ret;
		}

		/* Set init value */
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			ret = mfd_axp192_gpio_write_port(config->mfd, BIT(pin), 0);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			ret = mfd_axp192_gpio_write_port(config->mfd, BIT(pin), BIT(pin));
		}
	} else if ((flags & GPIO_INPUT) != 0) {

		/* Initialize input function */
		func = AXP192_GPIO_FUNC_INPUT;

		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, pin, func);
		if (ret != 0) {
			return ret;
		}

		/* Configure pull-down */
		if ((flags & GPIO_PULL_UP) != 0) {
			/* not supported */
			LOG_ERR("Pull-Up not supported");
			ret = -ENOTSUP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			/* out = 0 means pull-down*/
			ret = mfd_axp192_gpio_pd_ctrl(config->mfd, pin, true);
		} else {
			ret = mfd_axp192_gpio_pd_ctrl(config->mfd, pin, false);
		}
	} else {
		/* Neither input nor output mode is selected */
		LOG_INF("No valid gpio mode selected");
		ret = -ENOTSUP;
	}

	return ret;
}

static int gpio_axp192_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_axp192_data *data = dev->data;
	int ret;
	uint32_t value;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = gpio_axp192_port_get_raw(dev, &value);
	if (ret == 0) {
		ret = gpio_axp192_port_set_masked_raw(dev, pins, ~value);
	}

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int gpio_axp192_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

#if defined(CONFIG_GPIO_GET_CONFIG) || defined(CONFIG_GPIO_GET_DIRECTION)
static int gpio_axp192_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct gpio_axp192_config *config = dev->config;
	enum axp192_gpio_func func;
	bool pd_enabled;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = mfd_axp192_gpio_func_get(config->mfd, pin, &func);
	if (ret != 0) {
		return ret;
	}

	/* Set OUTPUT/INPUT flags */
	*out_flags = 0;
	switch (func) {
	case AXP192_GPIO_FUNC_INPUT:
		*out_flags |= GPIO_INPUT;
		break;
	case AXP192_GPIO_FUNC_OUTPUT_OD:
		*out_flags |= GPIO_OUTPUT | GPIO_OPEN_DRAIN;
		break;
	case AXP192_GPIO_FUNC_OUTPUT_LOW:
		*out_flags |= GPIO_OUTPUT;
		break;

	case AXP192_GPIO_FUNC_LDO:
		__fallthrough;
	case AXP192_GPIO_FUNC_ADC:
		__fallthrough;
	case AXP192_GPIO_FUNC_FLOAT:
		__fallthrough;
	default:
		LOG_DBG("Pin %d not configured as GPIO", pin);
		break;
	}

	/* Query pull-down config status  */
	ret = mfd_axp192_gpio_pd_get(config->mfd, pin, &pd_enabled);
	if (ret != 0) {
		return ret;
	}

	if (pd_enabled) {
		*out_flags |= GPIO_PULL_DOWN;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_axp192_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_axp192_config *config = dev->config;
	gpio_flags_t flags;
	int ret;

	/* reset output variables */
	*inputs = 0;
	*outputs = 0;

	/* loop through all  */
	for (gpio_pin_t gpio = 0; gpio < config->ngpios; gpio++) {

		if ((map & (1u << gpio)) != 0) {

			/* use internal get_config method to get gpio flags */
			ret = gpio_axp192_get_config(dev, gpio, &flags);
			if (ret != 0) {
				return ret;
			}

			/* Set output and input flags */
			if ((flags & GPIO_OUTPUT) != 0) {
				*outputs |= (1u << gpio);
			} else if (0 != (flags & GPIO_INPUT)) {
				*inputs |= (1u << gpio);
			}
		}
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static int gpio_axp192_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_axp192_data *const data = dev->data;

	return gpio_manage_callback(&data->cb_list_gpio, callback, set);
}

static const struct gpio_driver_api gpio_axp192_api = {
	.pin_configure = gpio_axp192_configure,
	.port_get_raw = gpio_axp192_port_get_raw,
	.port_set_masked_raw = gpio_axp192_port_set_masked_raw,
	.port_set_bits_raw = gpio_axp192_port_set_bits_raw,
	.port_clear_bits_raw = gpio_axp192_port_clear_bits_raw,
	.port_toggle_bits = gpio_axp192_port_toggle_bits,
	.pin_interrupt_configure = gpio_axp192_pin_interrupt_configure,
	.manage_callback = gpio_axp192_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_axp192_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_axp192_get_config,
#endif
};

static int gpio_axp192_init(const struct device *dev)
{
	const struct gpio_axp192_config *config = dev->config;
	struct gpio_axp192_data *data = dev->data;

	LOG_DBG("Initializing");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("device not ready");
		return -ENODEV;
	}

	return k_mutex_init(&data->mutex);
}

#define GPIO_AXP192_DEFINE(inst)                                                                   \
	static const struct gpio_axp192_config gpio_axp192_config##inst = {                        \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
	};                                                                                         \
                                                                                                   \
	static struct gpio_axp192_data gpio_axp192_data##inst;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &gpio_axp192_init, NULL, &gpio_axp192_data##inst,              \
			      &gpio_axp192_config##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_AXP192_INIT_PRIORITY, &gpio_axp192_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AXP192_DEFINE)
