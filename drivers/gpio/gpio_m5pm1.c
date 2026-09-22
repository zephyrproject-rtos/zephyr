/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m5stack_m5pm1_gpio

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/m5pm1.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(gpio_m5pm1, CONFIG_GPIO_LOG_LEVEL);

#define M5PM1_GPIO_NUM 5U

#define M5PM1_REG_GPIO_MODE  0x10
#define M5PM1_REG_GPIO_OUT   0x11
#define M5PM1_REG_GPIO_IN    0x12
#define M5PM1_REG_GPIO_DRV   0x13
#define M5PM1_REG_GPIO_PUPD0 0x14
#define M5PM1_REG_GPIO_PUPD1 0x15

/*
 * Pull-up/down occupies a 2-bit field per pin. Pins 0..3 are packed into the PUPD0 register; pin 4
 * lives alone at offset 0 of PUPD1.
 */
#define M5PM1_GPIO_PUPD_REG(pin)    ((pin) == 4U ? M5PM1_REG_GPIO_PUPD1 : M5PM1_REG_GPIO_PUPD0)
#define M5PM1_GPIO_FIELD_SHIFT(pin) (((pin) == 4U ? 0U : (pin)) * 2U)
#define M5PM1_GPIO_FIELD_MASK(pin)  (BIT_MASK(2) << M5PM1_GPIO_FIELD_SHIFT(pin))

#define M5PM1_GPIO_PULL_NONE 0U
#define M5PM1_GPIO_PULL_UP   1U
#define M5PM1_GPIO_PULL_DOWN 2U

struct gpio_m5pm1_config {
	struct gpio_driver_config common;
	const struct device *mfd;
	uint32_t ngpios;
};

struct gpio_m5pm1_data {
	struct gpio_driver_data common;
};

static int gpio_m5pm1_set_field(const struct device *mfd, uint8_t reg, uint8_t pin, uint8_t value)
{
	uint8_t shift = M5PM1_GPIO_FIELD_SHIFT(pin);
	uint8_t mask = M5PM1_GPIO_FIELD_MASK(pin);

	return mfd_m5pm1_update_reg(mfd, reg, mask, (uint8_t)(value << shift));
}

static int gpio_m5pm1_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_m5pm1_config *config = dev->config;
	uint8_t pull;
	int ret;

	if (pin >= config->ngpios) {
		return -EINVAL;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0 && (flags & GPIO_LINE_OPEN_DRAIN) == 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0 && (flags & GPIO_PULL_DOWN) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		pull = M5PM1_GPIO_PULL_UP;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		pull = M5PM1_GPIO_PULL_DOWN;
	} else {
		pull = M5PM1_GPIO_PULL_NONE;
	}

	ret = mfd_m5pm1_pin_request(config->mfd, dev, pin, M5PM1_PIN_FUNC_GPIO);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_m5pm1_set_field(config->mfd, M5PM1_GPIO_PUPD_REG(pin), pin, pull);
	if (ret < 0) {
		return ret;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		bool open_drain = (flags & GPIO_LINE_OPEN_DRAIN) != 0;

		ret = mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_DRV, BIT(pin),
					   open_drain ? BIT(pin) : 0U);
		if (ret < 0) {
			return ret;
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			ret = mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_OUT, BIT(pin),
						   BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			ret = mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_OUT, BIT(pin), 0U);
		} else {
			ret = 0;
		}

		if (ret < 0) {
			return ret;
		}

		return mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_MODE, BIT(pin), BIT(pin));
	}

	if ((flags & GPIO_INPUT) != 0) {
		return mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_MODE, BIT(pin), 0U);
	}

	return -ENOTSUP;
}

static int gpio_m5pm1_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_m5pm1_config *config = dev->config;
	uint8_t port;
	int ret;

	ret = mfd_m5pm1_read_reg(config->mfd, M5PM1_REG_GPIO_IN, &port);
	if (ret < 0) {
		return ret;
	}

	*value = port & GENMASK(M5PM1_GPIO_NUM - 1U, 0);

	return 0;
}

static int gpio_m5pm1_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_m5pm1_config *config = dev->config;

	mask &= GENMASK(M5PM1_GPIO_NUM - 1U, 0);
	value &= mask;

	return mfd_m5pm1_update_reg(config->mfd, M5PM1_REG_GPIO_OUT, mask, value);
}

static int gpio_m5pm1_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_m5pm1_port_set_masked_raw(dev, pins, pins);
}

static int gpio_m5pm1_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_m5pm1_port_set_masked_raw(dev, pins, 0);
}

static int gpio_m5pm1_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_m5pm1_config *config = dev->config;

	return mfd_m5pm1_toggle_reg(config->mfd, M5PM1_REG_GPIO_OUT,
				    pins & GENMASK(M5PM1_GPIO_NUM - 1U, 0));
}

static int gpio_m5pm1_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

static DEVICE_API(gpio, gpio_m5pm1_api) = {
	.pin_configure = gpio_m5pm1_configure,
	.port_get_raw = gpio_m5pm1_port_get_raw,
	.port_set_masked_raw = gpio_m5pm1_port_set_masked_raw,
	.port_set_bits_raw = gpio_m5pm1_port_set_bits_raw,
	.port_clear_bits_raw = gpio_m5pm1_port_clear_bits_raw,
	.port_toggle_bits = gpio_m5pm1_port_toggle_bits,
	.pin_interrupt_configure = gpio_m5pm1_pin_interrupt_configure,
};

static int gpio_m5pm1_init(const struct device *dev)
{
	const struct gpio_m5pm1_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR_DEVICE_NOT_READY(config->mfd);
		return -ENODEV;
	}

	return 0;
}

#define GPIO_M5PM1_DEFINE(inst)                                                                    \
	static const struct gpio_m5pm1_config gpio_m5pm1_config_##inst = {                         \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
	};                                                                                         \
	static struct gpio_m5pm1_data gpio_m5pm1_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, gpio_m5pm1_init, NULL, &gpio_m5pm1_data_##inst,                \
			      &gpio_m5pm1_config_##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_M5PM1_INIT_PRIORITY, &gpio_m5pm1_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_M5PM1_DEFINE)
