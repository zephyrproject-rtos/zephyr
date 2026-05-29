/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nordic_nrf_gswdt

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/watchdog.h>

/* Driver for Nordic Global Software Watchdog Timer (GSWDT)
 *
 * The GSWDT is a software-based watchdog that can be used in conjunction with
 * the MBOX peripheral to provide a watchdog functionality. The driver sends
 * messages to the system controller through the MBOX interface to feed the
 * watchdog. Maximum timeout is determined by the system controller configuration
 * and is set to 6s. Minimum feed interval should be 5s to avoid watchdog
 * expiration. Too short feeding intervals may cause increased power consumption.
 *
 * During wdt_nrf_gswdt_setup the driver sends a message to the system controller to start
 * the watchdog.
 * After that, the driver expects periodic calls to wdt_nrf_gswdt_feed to keep the watchdog from
 * expiring. If the watchdog expires, the system controller will reset the SoC.
 * Disable functionality is not supported, as the GSWDT cannot be disabled once started.
 */

 #define WDT_NRF_GSWDT_MAX_TIMEOUT_MS 5000

struct wdt_nrf_gswdt_config {
	struct mbox_dt_spec gswdt_mbox;
};

static int wdt_nrf_gswdt_setup(const struct device *dev, uint8_t options)
{
	if (options != 0U) {
		return -ENOTSUP;
	}

	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct wdt_nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_send_dt(gswdt_mbox, NULL);
}

static int wdt_nrf_gswdt_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -EPERM;
}

static int wdt_nrf_gswdt_install_timeout(const struct device *dev,
					 const struct wdt_timeout_cfg *cfg)
{
	ARG_UNUSED(dev);
	if (cfg->callback != NULL) {
		return -ENOTSUP;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U) {
		return -EINVAL;
	}

	if (cfg->window.max > WDT_NRF_GSWDT_MAX_TIMEOUT_MS) {
		return -EINVAL;
	}
	return 0;
}

static int wdt_nrf_gswdt_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);

	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct wdt_nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_send_dt(gswdt_mbox, NULL);
}

static DEVICE_API(wdt, wdt_nrf_gswdt_driver_api) = {
	.setup = wdt_nrf_gswdt_setup,
	.disable = wdt_nrf_gswdt_disable,
	.install_timeout = wdt_nrf_gswdt_install_timeout,
	.feed = wdt_nrf_gswdt_feed,
};

static int wdt_nrf_gswdt_init(const struct device *dev)
{
	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct wdt_nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_is_ready_dt(gswdt_mbox) ? 0 : -ENODEV;
}

#define GSWDT_INIT(inst)									\
	static struct wdt_nrf_gswdt_config wdt_nrf_gswdt_config_##inst = {			\
		.gswdt_mbox = MBOX_DT_SPEC_GET(DT_PHANDLE(DT_INST(inst, nordic_nrf_gswdt),	\
		mbox), tx)									\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
	wdt_nrf_gswdt_init,									\
	NULL,											\
	NULL,											\
	&wdt_nrf_gswdt_config_##inst,								\
	POST_KERNEL,										\
	UTIL_INC(CONFIG_MBOX_INIT_PRIORITY),							\
	&wdt_nrf_gswdt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GSWDT_INIT)
