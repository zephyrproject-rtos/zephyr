/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm2100_wdt

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/mfd/npm2100.h>

#define TIMER_TASKS_START 0xB0U
#define TIMER_TASKS_STOP  0xB1U
#define TIMER_TASKS_KICK  0xB2U
#define TIMER_CONFIG      0xB3U
#define TIMER_TARGET_HI   0xB4U
#define TIMER_TARGET_MID  0xB5U
#define TIMER_TARGET_LO   0xB6U
#define TIMER_STATUS      0xB7U
#define TIMER_BOOT_MON    0xB8U

struct wdt_npm2100_config {
	const struct device *mfd;
	struct i2c_dt_spec i2c;
};

struct wdt_npm2100_data {
	bool timeout_valid;
};

static int wdt_npm2100_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_npm2100_config *config = dev->config;
	struct wdt_npm2100_data *data = dev->data;

	if (!data->timeout_valid) {
		return -EINVAL;
	}

	return mfd_npm2100_start_timer(config->mfd);
}

static int wdt_npm2100_disable(const struct device *dev)
{
	const struct wdt_npm2100_config *config = dev->config;
	struct wdt_npm2100_data *data = dev->data;
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, TIMER_TASKS_STOP, 1U);
	if (ret < 0) {
		return ret;
	}

	data->timeout_valid = false;

	return 0;
}

static int wdt_npm2100_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *timeout)
{
	const struct wdt_npm2100_config *config = dev->config;
	struct wdt_npm2100_data *data = dev->data;
	uint8_t mode;
	int ret;

	if (data->timeout_valid) {
		return -ENOMEM;
	}

	if (timeout->window.min != 0U) {
		return -EINVAL;
	}

	switch (timeout->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_NONE:
		/* Watchdog expiry causes event only, and does not reset */
		mode = NPM2100_TIMER_MODE_GENERAL_PURPOSE;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		/* Watchdog expiry asserts reset output */
		mode = NPM2100_TIMER_MODE_WDT_RESET;
		break;
	case WDT_FLAG_RESET_SOC:
		/* Watchdog expiry causes full power cycle */
		mode = NPM2100_TIMER_MODE_WDT_POWER_CYCLE;
		break;
	default:
		return -EINVAL;
	}

	ret = mfd_npm2100_set_timer(config->mfd, timeout->window.max, mode);
	if (ret < 0) {
		return ret;
	}

	data->timeout_valid = true;

	return 0;
}

static int wdt_npm2100_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npm2100_config *config = dev->config;

	if (channel_id != 0) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, TIMER_TASKS_KICK, 1U);
}

static DEVICE_API(wdt, wdt_npm2100_api) = {
	.setup = wdt_npm2100_setup,
	.disable = wdt_npm2100_disable,
	.install_timeout = wdt_npm2100_install_timeout,
	.feed = wdt_npm2100_feed,
};

static int wdt_npm2100_init(const struct device *dev)
{
	const struct wdt_npm2100_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* Disable boot monitor */
	return wdt_npm2100_disable(dev);
}

#define WDT_NPM2100_DEFINE(n)                                                                      \
	static struct wdt_npm2100_data data##n;                                                    \
                                                                                                   \
	static const struct wdt_npm2100_config config##n = {                                       \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &wdt_npm2100_init, NULL, &data##n, &config##n, POST_KERNEL,       \
			      CONFIG_WDT_NPM2100_INIT_PRIORITY, &wdt_npm2100_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_NPM2100_DEFINE)
