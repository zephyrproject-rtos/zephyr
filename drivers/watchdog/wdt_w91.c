/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <ipc/ipc_based_driver.h>
#include <stdint.h>
#include <stdbool.h>

#define DT_DRV_COMPAT telink_w91_watchdog

#define LOG_MODULE_NAME watchdog_w91
#if defined(CONFIG_WDT_LOG_LEVEL)
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct wdt_w91_config {
	uint8_t instance_id;    /* instance id */
};

struct wdt_w91_data {
	struct ipc_based_driver ipc;   /* ipc driver part */
	struct wdt_timeout_cfg timeout_config;
	uint64_t timeout_ms;
	bool timeout_installed;
	bool is_enabled;
};

/* Type defines for the watchdog functions IPC API */
enum {
	IPC_DISPATCHER_WATCHDOG_START = IPC_DISPATCHER_WATCHDOG,
	IPC_DISPATCHER_WATCHDOG_STOP,
	IPC_DISPATCHER_WATCHDOG_RESET,
	IPC_DISPATCHER_WATCHDOG_EXPIRE_NOW,
};


/* Free RTOS watchdog functions IPC API */
static int pack_watchdog_ipc_start(
	uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint64_t *p_timeout_ms = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(*p_timeout_ms);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WATCHDOG_START, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, *p_timeout_ms);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(watchdog_ipc_start);

static int watchdog_ipc_start(const struct device *dev, uint64_t timeout_ms)
{
	int err = -ETIME;
	struct ipc_based_driver *ipc_data = &((struct wdt_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct wdt_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, watchdog_ipc_start,
			&timeout_ms, &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}


IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(watchdog_ipc_stop,
				IPC_DISPATCHER_WATCHDOG_STOP);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(watchdog_ipc_stop);

static int watchdog_ipc_stop(const struct device *dev)
{
	int err = -ETIME;
	struct ipc_based_driver *ipc_data = &((struct wdt_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct wdt_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, watchdog_ipc_stop,
				NULL, &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(watchdog_ipc_reset,
			IPC_DISPATCHER_WATCHDOG_RESET);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(watchdog_ipc_reset);

static int watchdog_ipc_reset(const struct device *dev)
{
	int err = -ETIME;
	struct ipc_based_driver *ipc_data = &((struct wdt_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct wdt_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, watchdog_ipc_reset,
			NULL, &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}


IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(watchdog_ipc_wrap_expire_now,
			IPC_DISPATCHER_WATCHDOG_EXPIRE_NOW);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(watchdog_ipc_wrap_expire_now);

static int watchdog_ipc_wrap_expire_now(const struct device *dev)
{
	int err = -ETIME;
	struct ipc_based_driver *ipc_data = &((struct wdt_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct wdt_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, watchdog_ipc_wrap_expire_now,
				NULL, &err, CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* Static functions daclaration */
#if CONFIG_WDT_ALLOW_CALLBACK
static void run_wd_callback(wdt_callback_t cb_ptr, const struct device *dev)
{
	if (cb_ptr != NULL) {
		cb_ptr(dev, 0);
	}
}
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */

static void clear_wd_config(const struct device *dev)
{
	if (dev != NULL) {
		struct wdt_w91_data *wdt_w91_data = (struct wdt_w91_data *)dev->data;
		struct wdt_timeout_cfg *timeout_cfg = &wdt_w91_data->timeout_config;

		wdt_w91_data->timeout_ms = 0;
		wdt_w91_data->timeout_installed = false;
		wdt_w91_data->is_enabled = false;
		timeout_cfg->window.min = 0;
		timeout_cfg->window.max = 0;
#if CONFIG_WDT_ALLOW_CALLBACK
		timeout_cfg->callback = NULL;
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */
		timeout_cfg->flags = WDT_FLAG_RESET_NONE;
	} else {
		LOG_ERR("Watchdog data clear not done");
	}
}

/* Zephyr watchdog API */
static int wdt_w91_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(options);

	int result = 0;
	struct wdt_w91_data *wdt_w91_data = (struct wdt_w91_data *)dev->data;
	struct wdt_timeout_cfg *timeout_cfg = &wdt_w91_data->timeout_config;

	if (wdt_w91_data->is_enabled == true) {
		LOG_WRN("Watchdog is busy");
		result = -EBUSY;
	} else if ((wdt_w91_data->is_enabled == false) &&
				(wdt_w91_data->timeout_installed == true)) {
		if (timeout_cfg->flags == WDT_FLAG_RESET_NONE) {
#if CONFIG_WDT_ALLOW_CALLBACK
			run_wd_callback(timeout_cfg->callback, dev);
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */
		} else if (timeout_cfg->flags == WDT_FLAG_RESET_SOC &&
					wdt_w91_data->timeout_ms == 0) {
#if CONFIG_WDT_ALLOW_CALLBACK
			run_wd_callback(timeout_cfg->callback, dev);
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */
			result = watchdog_ipc_wrap_expire_now(dev);
		} else if (timeout_cfg->flags == WDT_FLAG_RESET_SOC &&
					wdt_w91_data->timeout_ms != 0) {
#if CONFIG_WDT_ALLOW_CALLBACK
			run_wd_callback(timeout_cfg->callback, dev);
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */
			result = watchdog_ipc_start(dev, wdt_w91_data->timeout_ms);
		} else {
			result = -ENOTSUP;
			LOG_WRN("Watchdog config not supported");
		}
	}

	if (result == 0) {
		wdt_w91_data->is_enabled = true;
		LOG_INF("HW watchdog started");
	}

	return result;
}

static int wdt_w91_disable(const struct device *dev)
{
	int result = 0;
	struct wdt_w91_data *wdt_w91_data = (struct wdt_w91_data *)dev->data;

	if (wdt_w91_data->is_enabled == false) {
		result = -EFAULT;
		LOG_INF("Watchdog instance is not enabled");
	} else {
		result = watchdog_ipc_stop(dev);
		clear_wd_config(dev);
		LOG_INF("Watchdog stopped");
	}

	return result;
}

static int wdt_w91_install_timeout(const struct device *dev,
			const struct wdt_timeout_cfg *cfg)
{
	int result = 0;
	struct wdt_w91_data *wdt_w91_data = (struct wdt_w91_data *)dev->data;
	struct wdt_timeout_cfg *timeout_cfg = &wdt_w91_data->timeout_config;

	if ((cfg != NULL) && (wdt_w91_data->timeout_installed == false) &&
		(wdt_w91_data->is_enabled == false)) {
		timeout_cfg->window.min = cfg->window.min;
		timeout_cfg->window.max = cfg->window.max;
#if CONFIG_WDT_ALLOW_CALLBACK
		timeout_cfg->callback = cfg->callback;
#endif    /* CONFIG_WDT_ALLOW_CALLBACK */
		timeout_cfg->flags = cfg->flags;
	} else if (wdt_w91_data->timeout_installed == true) {
		LOG_WRN("Watchdog has been already installed");
		result = -ENOMEM;
	} else if (wdt_w91_data->is_enabled == true) {
		LOG_WRN("Watchdog is busy");
		result = -EBUSY;
	} else {
		LOG_WRN("Watchdog wrong timeout config argument");
		result = -ETIME;
	}

	if (result == 0) {
		if (timeout_cfg->window.min != 0) {
			LOG_WRN("Window watchdog not supported");
			result = -EINVAL;
		} else if (timeout_cfg->flags != WDT_FLAG_RESET_SOC &&
					timeout_cfg->flags != WDT_FLAG_RESET_NONE) {
			LOG_WRN("Watchdog not supported flag");
			result = -EINVAL;
		} else {
			wdt_w91_data->timeout_ms = timeout_cfg->window.max;
			wdt_w91_data->timeout_installed = true;
			LOG_INF("Watchdog timeout installed to %llu ms", wdt_w91_data->timeout_ms);
		}
	}

	return result;
}

static int wdt_w91_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);

	LOG_INF("Watchdog feed");

	return watchdog_ipc_reset(dev);
}

static const struct wdt_driver_api wdt_w91_api = {
	.setup = wdt_w91_setup,
	.disable = wdt_w91_disable,
	.install_timeout = wdt_w91_install_timeout,
	.feed = wdt_w91_feed,
};

static int wdt_w91_init(const struct device *dev)
{
	struct wdt_w91_data *wdt_w91_data = (struct wdt_w91_data *)dev->data;

	clear_wd_config(dev);
	ipc_based_driver_init(&wdt_w91_data->ipc);

	return 0;
}

/* WDT driver registration */
#define WDT_W91_INIT(n)                                            \
                                                                   \
static const struct wdt_w91_config wdt_w91_config_##n = {        \
	.instance_id = n,                                              \
};                                                                 \
                                                                   \
static struct wdt_w91_data wdt_w91_data_##n;                     \
                                                                   \
DEVICE_DT_INST_DEFINE(n, wdt_w91_init, NULL, &wdt_w91_data_##n,    \
					&wdt_w91_config_##n, PRE_KERNEL_1,             \
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE,            \
					&wdt_w91_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_W91_INIT)
