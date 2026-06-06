/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m5stack_m5pm1_wdt

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/m5pm1.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(wdt_m5pm1, CONFIG_WDT_LOG_LEVEL);

#define M5PM1_REG_WDT_CNT 0x0a
#define M5PM1_REG_WDT_KEY 0x0b

/* Writing this key to WDT_KEY clears and reloads the countdown (feeds the watchdog). */
#define M5PM1_WDT_KEY_FEED 0xa5

/* WDT_CNT is an 8-bit countdown in seconds; 0 disables the watchdog. */
#define M5PM1_WDT_MAX_SECONDS UINT8_MAX

struct wdt_m5pm1_config {
	const struct device *mfd;
};

struct wdt_m5pm1_data {
	uint8_t timeout;
	bool timeout_valid;
	bool enabled;
};

static int wdt_m5pm1_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_m5pm1_config *config = dev->config;
	struct wdt_m5pm1_data *data = dev->data;
	int ret;

	if (!data->timeout_valid) {
		return -EINVAL;
	}

	if (data->enabled) {
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		return -ENOTSUP;
	}

	/* Writing a non-zero second count starts the countdown. */
	ret = mfd_m5pm1_write_reg(config->mfd, M5PM1_REG_WDT_CNT, data->timeout);
	if (ret < 0) {
		return ret;
	}

	data->enabled = true;

	return 0;
}

static int wdt_m5pm1_disable(const struct device *dev)
{
	const struct wdt_m5pm1_config *config = dev->config;
	struct wdt_m5pm1_data *data = dev->data;
	int ret;

	ret = mfd_m5pm1_write_reg(config->mfd, M5PM1_REG_WDT_CNT, 0U);
	if (ret < 0) {
		return ret;
	}

	data->enabled = false;
	data->timeout_valid = false;

	return 0;
}

static int wdt_m5pm1_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *timeout)
{
	struct wdt_m5pm1_data *data = dev->data;
	uint32_t seconds;

	if (data->timeout_valid) {
		return -ENOMEM;
	}

	/* No windowed timeout and no pre-reset warning interrupt on this hardware. */
	if (timeout->window.min != 0U) {
		return -EINVAL;
	}

	if (timeout->callback != NULL) {
		return -ENOTSUP;
	}

	/* The watchdog always forces a full system reset on expiry. */
	switch (timeout->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_SOC:
	case WDT_FLAG_RESET_CPU_CORE:
		break;
	default:
		return -ENOTSUP;
	}

	seconds = DIV_ROUND_UP(timeout->window.max, MSEC_PER_SEC);
	if (seconds == 0U || seconds > M5PM1_WDT_MAX_SECONDS) {
		return -EINVAL;
	}

	data->timeout = (uint8_t)seconds;
	data->timeout_valid = true;

	return 0;
}

static int wdt_m5pm1_feed(const struct device *dev, int channel_id)
{
	const struct wdt_m5pm1_config *config = dev->config;
	struct wdt_m5pm1_data *data = dev->data;

	if (channel_id != 0) {
		return -EINVAL;
	}

	if (!data->enabled) {
		return -EINVAL;
	}

	return mfd_m5pm1_write_reg(config->mfd, M5PM1_REG_WDT_KEY, M5PM1_WDT_KEY_FEED);
}

static DEVICE_API(wdt, wdt_m5pm1_api) = {
	.setup = wdt_m5pm1_setup,
	.disable = wdt_m5pm1_disable,
	.install_timeout = wdt_m5pm1_install_timeout,
	.feed = wdt_m5pm1_feed,
};

static int wdt_m5pm1_init(const struct device *dev)
{
	const struct wdt_m5pm1_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR_DEVICE_NOT_READY(config->mfd);
		return -ENODEV;
	}

	return 0;
}

#define WDT_M5PM1_DEFINE(inst)                                                                     \
	static const struct wdt_m5pm1_config wdt_m5pm1_config_##inst = {                           \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
	static struct wdt_m5pm1_data wdt_m5pm1_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, wdt_m5pm1_init, NULL, &wdt_m5pm1_data_##inst,                  \
			      &wdt_m5pm1_config_##inst, POST_KERNEL,                               \
			      CONFIG_WDT_M5PM1_INIT_PRIORITY, &wdt_m5pm1_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_M5PM1_DEFINE)
