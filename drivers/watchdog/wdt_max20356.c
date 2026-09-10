/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max20356_watchdog

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/max20356.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/util.h>

#include "mfd_max20356.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_max20356, CONFIG_WDT_LOG_LEVEL);

static const uint32_t wdt_max20356_intervals_ms[] = {4000U, 8000U, 16000U, 32000U};

struct wdt_max20356_config {
	const struct device *mfd;
	bool lock_enable;
};

struct wdt_max20356_data {
	uint8_t tmrsel;
	uint8_t rsttype;
	bool timeout_valid;
	bool enabled;
};

static int wdt_max20356_reg_update(const struct device *dev, uint8_t mask, uint8_t val)
{
	const struct wdt_max20356_config *config = dev->config;

	if (config->lock_enable) {
		return mfd_max20356_reg_update_locked(config->mfd, MAX20356_LOCK_WD,
						      MAX20356_REG_WDCNTL, mask, val);
	}

	return mfd_max20356_reg_update(config->mfd, MAX20356_REG_WDCNTL, mask, val);
}

static int wdt_max20356_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *timeout)
{
	struct wdt_max20356_data *data = dev->data;
	uint8_t tmrsel;

	if (data->timeout_valid) {
		LOG_ERR("Timeout already installed; only one is supported");
		return -ENOMEM;
	}

	/* Hardware has no windowed mode and no pre-expiry warning interrupt. */
	if (timeout->window.min != 0U) {
		LOG_ERR("Windowed mode not supported (window.min must be 0)");
		return -EINVAL;
	}

	if (timeout->callback != NULL) {
		LOG_ERR("Callback not supported; no pre-expiry warning interrupt");
		return -ENOTSUP;
	}

	/* Map window.max to the smallest interval that is >= the request. */
	for (tmrsel = 0U; tmrsel < ARRAY_SIZE(wdt_max20356_intervals_ms); tmrsel++) {
		if (timeout->window.max <= wdt_max20356_intervals_ms[tmrsel]) {
			break;
		}
	}

	if (tmrsel >= ARRAY_SIZE(wdt_max20356_intervals_ms)) {
		LOG_ERR("Timeout %u ms exceeds maximum of %u ms", timeout->window.max,
			wdt_max20356_intervals_ms[ARRAY_SIZE(wdt_max20356_intervals_ms) - 1U]);
		return -EINVAL;
	}

	switch (timeout->flags & WDT_FLAG_RESET_MASK) {
	case WDT_FLAG_RESET_SOC:
		data->rsttype = MAX20356_WDT_HARD_RESET;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		data->rsttype = MAX20356_WDT_SOFT_RESET;
		break;
	default:
		LOG_ERR("Reset flag not supported; use RESET_SOC or RESET_CPU_CORE");
		return -ENOTSUP;
	}

	data->tmrsel = tmrsel;
	data->timeout_valid = true;

	return 0;
}

static int wdt_max20356_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_max20356_config *config = dev->config;
	struct wdt_max20356_data *data = dev->data;
	int ret;

	if (!data->timeout_valid) {
		LOG_ERR("No timeout installed; call wdt_install_timeout() first");
		return -EINVAL;
	}

	if (data->enabled) {
		LOG_ERR("Watchdog already running");
		return -EBUSY;
	}

	/* The watchdog runs only in the On power mode and cannot be paused */
	if ((options & (WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG)) != 0U) {
		LOG_ERR("Pause options not supported (0x%02x)", options);
		return -ENOTSUP;
	}

	/* The feed reads the shared Int5 register, so the watchdog must own INTB
	 * exclusively: claim it before arming, refusing if an INTB consumer is
	 * already registered.
	 */
	ret = mfd_max20356_wdt_claim(config->mfd, true);
	if (ret != 0) {
		LOG_ERR("Failed to claim exclusive INTB ownership: %d", ret);
		return ret;
	}

	/* set WDRstType = 0 before changing WDTmrSel */
	ret = wdt_max20356_reg_update(dev,
				      MAX20356_WDCNTL_WDRSTTYPE_MSK | MAX20356_WDCNTL_WDTMRSEL_MSK,
				      FIELD_PREP(MAX20356_WDCNTL_WDTMRSEL_MSK, data->tmrsel));
	if (ret != 0) {
		LOG_ERR("Failed to set WDTmrSel: %d", ret);
		mfd_max20356_wdt_claim(config->mfd, false);
		return ret;
	}

	ret = wdt_max20356_reg_update(dev, MAX20356_WDCNTL_WDRSTTYPE_MSK,
				      FIELD_PREP(MAX20356_WDCNTL_WDRSTTYPE_MSK, data->rsttype));
	if (ret != 0) {
		LOG_ERR("Failed to arm watchdog (WDRstType): %d", ret);
		mfd_max20356_wdt_claim(config->mfd, false);
		return ret;
	}

	data->enabled = true;

	return 0;
}

static int wdt_max20356_disable(const struct device *dev)
{
	const struct wdt_max20356_config *config = dev->config;
	struct wdt_max20356_data *data = dev->data;
	int ret;

	if (!data->enabled) {
		LOG_ERR("Watchdog not running");
		return -EFAULT;
	}

	ret = wdt_max20356_reg_update(dev, MAX20356_WDCNTL_WDRSTTYPE_MSK,
				      FIELD_PREP(MAX20356_WDCNTL_WDRSTTYPE_MSK, MAX20356_WDT_OFF));
	if (ret != 0) {
		LOG_ERR("Failed to stop watchdog (WDRstType): %d", ret);
		return ret;
	}

	ret = mfd_max20356_wdt_claim(config->mfd, false);
	if (ret != 0) {
		LOG_ERR("Failed to release INTB ownership: %d", ret);
		return ret;
	}

	data->enabled = false;
	data->timeout_valid = false;

	return 0;
}

static int wdt_max20356_feed(const struct device *dev, int channel_id)
{
	const struct wdt_max20356_config *config = dev->config;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel %d; only channel 0 exists", channel_id);
		return -EINVAL;
	}

	return mfd_max20356_wdt_feed(config->mfd);
}

static DEVICE_API(wdt, wdt_max20356_driver_api) = {
	.setup = wdt_max20356_setup,
	.disable = wdt_max20356_disable,
	.install_timeout = wdt_max20356_install_timeout,
	.feed = wdt_max20356_feed,
};

static int wdt_max20356_init(const struct device *dev)
{
	const struct wdt_max20356_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR("MFD parent device not ready");
		return -ENODEV;
	}

	return 0;
}

#define WDT_MAX20356_DEFINE(inst)                                                                  \
	static struct wdt_max20356_data wdt_max20356_data_##inst;                                  \
                                                                                                   \
	static const struct wdt_max20356_config wdt_max20356_config_##inst = {                     \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.lock_enable = DT_INST_PROP(inst, adi_lock_enable),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &wdt_max20356_init, NULL, &wdt_max20356_data_##inst,           \
			      &wdt_max20356_config_##inst, POST_KERNEL,                            \
			      CONFIG_WDT_MAX20356_INIT_PRIORITY, &wdt_max20356_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_MAX20356_DEFINE)
