/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nordic_nrf_gswdt

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/watchdog.h>
#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
#include <nrfs_backend_ipc_service.h>
#include <nrfs_gswdt.h>
#endif

/* Driver for Nordic Global Software Watchdog Timer (GSWDT)
 *
 * The GSWDT is a software-based watchdog that can be used in conjunction with
 * the MBOX peripheral to provide a watchdog functionality. The driver sends
 * messages to the system controller through the MBOX interface to feed the
 * watchdog.
 * Maximum timeout can be configured using gswdt service if enabled, otherwise it is fixed
 * and is set to 6s. Minimum feed interval should be less than the maximum timeout
 * to avoid watchdog expiration. Too short feeding intervals may cause increased
 * power consumption.
 *
 * During wdt_nrf_gswdt_setup the driver sends a message to the system controller to start
 * the watchdog.
 * After that, the driver expects periodic calls to wdt_nrf_gswdt_feed to keep the watchdog from
 * expiring. If the watchdog expires, the system controller will reset the SoC.
 * Disable functionality is not supported, as the GSWDT cannot be disabled once started.
 *
 * This watchdog timer can be disabled only if the gswdt service is enabled otherwise it cannot
 * be disabled once started.
 */

 #define WDT_NRF_GSWDT_DEFAULT_TIMEOUT_MS 6000

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

#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
	nrfs_err_t err = nrfs_gswdt_stop(NULL);

	if (err != NRFS_SUCCESS) {
		return -EIO;
	}
	return 0;
#endif
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

#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
	nrfs_err_t status = nrfs_gswdt_timeout_set(cfg->window.max, NULL);

	if (status != NRFS_SUCCESS) {
		return -EIO;
	}
#else
	if (cfg->window.max != WDT_NRF_GSWDT_DEFAULT_TIMEOUT_MS) {
		return -EINVAL;
	}
#endif
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

#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
static void wdt_nrf_nrfs_gswdt_handler(nrfs_gswdt_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_GSWDT_EVT_TIMEOUT_SET:
		break;
	case NRFS_GSWDT_EVT_STOP_DONE:
		break;
	case NRFS_GSWDT_EVT_REJECT:
		break;
	default:
		break;
	}
}
#endif

static int wdt_nrf_gswdt_init(const struct device *dev)
{
	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct wdt_nrf_gswdt_config *)dev->config)->gswdt_mbox;

	int ret = mbox_is_ready_dt(gswdt_mbox) ? 0 : -ENODEV;

	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
	nrfs_err_t err;

	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		printk("%s %s", "nrfs backend connection", "failed\n");
		return -EIO;
	}

	err = nrfs_gswdt_init(wdt_nrf_nrfs_gswdt_handler);
	if (err != NRFS_SUCCESS) {
		printk("%s %s", "nrfs gswdt init", "failed\n");
		return -EIO;
	}
#endif
	return ret;
}

#if defined(CONFIG_NRFS_GSWDT_SERVICE_ENABLED)
#define GSWDT_INIT_PRIORITY UTIL_INC(CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO)
#else
#define GSWDT_INIT_PRIORITY UTIL_INC(CONFIG_MBOX_INIT_PRIORITY)
#endif

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
	GSWDT_INIT_PRIORITY,									\
	&wdt_nrf_gswdt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GSWDT_INIT)
