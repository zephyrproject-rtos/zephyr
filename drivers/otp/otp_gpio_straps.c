/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#define DT_DRV_COMPAT zephyr_gpio_straps

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(otp_gpio_straps, CONFIG_OTP_LOG_LEVEL);

struct gpio_straps_config {
	const struct gpio_dt_spec *gpios;
	size_t num_gpios;
	size_t size;
};

static int gpio_straps_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct gpio_straps_config *config = dev->config;
	const uint8_t *value = dev->data;

	if (offset < 0 || len > config->size || (size_t)offset > config->size - len) {
		return -EINVAL;
	}

	memcpy(data, value + offset, len);

	return 0;
}

static int gpio_straps_init(const struct device *dev)
{
	const struct gpio_straps_config *config = dev->config;
	uint8_t *value = dev->data;
	int ret;

	/*
	 * Configure every strap before sampling any of them, so a pin biased by
	 * its own pull resistor has settled by the time it is read.
	 */
	for (size_t i = 0; i < config->num_gpios; i++) {
		if (!gpio_is_ready_dt(&config->gpios[i])) {
			LOG_ERR("strap %zu: GPIO controller not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->gpios[i], GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("strap %zu: configure failed (%d)", i, ret);
			return ret;
		}
	}

	for (size_t i = 0; i < config->num_gpios; i++) {
		ret = gpio_pin_get_dt(&config->gpios[i]);
		if (ret < 0) {
			LOG_ERR("strap %zu: read failed (%d)", i, ret);
			return ret;
		}

		WRITE_BIT(value[i / 8U], i % 8U, ret);

		/*
		 * Release the pin now that its bit is captured, rather than
		 * pass the devicetree flags to gpio_pin_configure_dt() and keep
		 * the pull applied. Some controllers also reject a bias on a
		 * disconnected pin.
		 */
		ret = gpio_pin_configure(config->gpios[i].port, config->gpios[i].pin,
					 GPIO_DISCONNECTED);
		if (ret != 0) {
			LOG_DBG("strap %zu: not released (%d)", i, ret);
		}
	}

	return 0;
}

static DEVICE_API(otp, gpio_straps_driver_api) = {
	.read = gpio_straps_read,
};

#define GPIO_STRAPS_INIT(n)                                                                        \
	static const struct gpio_dt_spec gpio_straps_pins_##n[] = {                                \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};            \
                                                                                                   \
	static uint8_t gpio_straps_value_##n[DIV_ROUND_UP(DT_INST_PROP_LEN(n, gpios), 8)];         \
                                                                                                   \
	static const struct gpio_straps_config gpio_straps_config_##n = {                          \
		.gpios = gpio_straps_pins_##n,                                                     \
		.num_gpios = DT_INST_PROP_LEN(n, gpios),                                           \
		.size = DIV_ROUND_UP(DT_INST_PROP_LEN(n, gpios), 8),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_straps_init, NULL, gpio_straps_value_##n,                    \
			      &gpio_straps_config_##n, POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,      \
			      &gpio_straps_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_STRAPS_INIT)
