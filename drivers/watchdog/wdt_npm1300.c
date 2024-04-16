/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_wdt

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/dt-bindings/gpio/nordic-npm1300-gpio.h>

/* nPM1300 TIMER base address */
#define TIME_BASE 0x07U

/* nPM1300 timer register offsets */
#define TIME_OFFSET_START     0x00U
#define TIME_OFFSET_STOP      0x01U
#define TIME_OFFSET_WDOG_KICK 0x04U
#define TIME_OFFSET_MODE      0x05U

/* nPM1300 timer modes */
#define TIME_MODE_BOOT  0x00U
#define TIME_MODE_WARN  0x01U
#define TIME_MODE_RESET 0x02U
#define TIME_MODE_GEN   0x03U

struct wdt_npm1300_config {
	const struct device *mfd;
	struct gpio_dt_spec reset_gpios;
};

struct wdt_npm1300_data {
	bool timeout_valid;
};

static int wdt_npm1300_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_npm1300_config *config = dev->config;
	struct wdt_npm1300_data *data = dev->data;

	if (!data->timeout_valid) {
		return -EINVAL;
	}

	return mfd_npm1300_reg_write(config->mfd, TIME_BASE, TIME_OFFSET_START, 1U);
}

static int wdt_npm1300_disable(const struct device *dev)
{
	const struct wdt_npm1300_config *config = dev->config;
	struct wdt_npm1300_data *data = dev->data;
	int ret;

	ret = mfd_npm1300_reg_write(config->mfd, TIME_BASE, TIME_OFFSET_STOP, 1U);
	if (ret < 0) {
		return ret;
	}

	data->timeout_valid = false;

	return 0;
}

static int wdt_npm1300_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *timeout)
{
	const struct wdt_npm1300_config *config = dev->config;
	struct wdt_npm1300_data *data = dev->data;
	uint8_t mode;
	int ret;

	if (data->timeout_valid) {
		return -ENOMEM;
	}

	if (timeout->window.min != 0U) {
		return -EINVAL;
	}

	ret = mfd_npm1300_set_timer(config->mfd, timeout->window.max);
	if (ret < 0) {
		return ret;
	}

	switch (timeout->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_NONE:
		/* Watchdog expiry causes warn event only, and does not reset */
		mode = TIME_MODE_GEN;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		/* Watchdog expiry causes warn event, then asserts reset output */
		mode = TIME_MODE_WARN;
		break;
	case WDT_FLAG_RESET_SOC:
		/* Watchdog expiry causes warn event, then full power cycle */
		mode = TIME_MODE_RESET;
		break;
	default:
		return -EINVAL;
	}

	ret = mfd_npm1300_reg_write(config->mfd, TIME_BASE, TIME_OFFSET_MODE, mode);
	if (ret < 0) {
		return ret;
	}

	data->timeout_valid = true;

	return 0;
}

static int wdt_npm1300_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npm1300_config *config = dev->config;

	if (channel_id != 0) {
		return -EINVAL;
	}

	return mfd_npm1300_reg_write(config->mfd, TIME_BASE, TIME_OFFSET_WDOG_KICK, 1U);
}

static const struct wdt_driver_api wdt_npm1300_api = {
	.setup = wdt_npm1300_setup,
	.disable = wdt_npm1300_disable,
	.install_timeout = wdt_npm1300_install_timeout,
	.feed = wdt_npm1300_feed,
};

static int wdt_npm1300_init(const struct device *dev)
{
	const struct wdt_npm1300_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	if (config->reset_gpios.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpios)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpios, NPM1300_GPIO_WDT_RESET_ON);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#define WDT_NPM1300_DEFINE(n)                                                                      \
	static struct wdt_npm1300_data data##n;                                                    \
                                                                                                   \
	static const struct wdt_npm1300_config config##n = {                                       \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &wdt_npm1300_init, NULL, &data##n, &config##n, POST_KERNEL,       \
			      CONFIG_WDT_NPM1300_INIT_PRIORITY, &wdt_npm1300_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_NPM1300_DEFINE)
