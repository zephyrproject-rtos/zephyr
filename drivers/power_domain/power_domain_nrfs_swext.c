/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrfs_swext

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <nrfs_swext.h>
#include <nrfs_backend_ipc_service.h>

LOG_MODULE_REGISTER(nrfs_swext, CONFIG_POWER_DOMAIN_LOG_LEVEL);

BUILD_ASSERT(
	DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	"multiple instances not supported"
);

struct nrfs_swext_data {
	struct k_sem evt_sem;
	nrfs_swext_evt_type_t evt;
};

struct nrfs_swext_config {
	uint16_t current_limit_ua;
	bool enable_power_down_clamp;
};

static void nrfs_swext_driver_evt_handler(nrfs_swext_evt_t const *p_evt, void *context)
{
	struct nrfs_swext_data *dev_data = context;

	LOG_DBG("evt %u", (uint32_t)p_evt->type);

	if (p_evt->type == NRFS_SWEXT_EVT_OVERCURRENT) {
		/* Overcurrent is an unrecoverable condition which requires hardware fix */
		LOG_ERR("overcurrent");
		k_panic();
	};

	dev_data->evt = p_evt->type;
	k_sem_give(&dev_data->evt_sem);
}

static int nrfs_swext_driver_power_down(const struct device *dev)
{
	const struct nrfs_swext_config *dev_config = dev->config;
	nrfs_err_t err;
	swext_pd_clamp_t pd_clamp = dev_config->enable_power_down_clamp
				  ? SWEXT_PD_CLAMP_ENABLED
				  : SWEXT_PD_CLAMP_DISABLED;

	/*
	 * Power down request does not respond with an event.
	 * Set context to NULL, fire and forget.
	 */
	err = nrfs_swext_power_down(pd_clamp, NULL);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to request power down");
		return -ENODEV;
	}

	return 0;
}

static int nrfs_swext_driver_power_up(const struct device *dev)
{
	struct nrfs_swext_data *dev_data = dev->data;
	const struct nrfs_swext_config *dev_config = dev->config;
	nrfs_err_t err;
	uint8_t load_current;

	load_current = nrfs_swext_load_current_to_raw(dev_config->current_limit_ua);
	err = nrfs_swext_power_up(load_current, dev_data);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to request power up");
		return -ENODEV;
	}

	(void)k_sem_take(&dev_data->evt_sem, K_FOREVER);

	if (dev_data->evt == NRFS_SWEXT_EVT_ENABLED) {
		return 0;
	}

	LOG_ERR("power up request rejected");
	return -EIO;
}

#if IS_ENABLED(CONFIG_DEVICE_DEPS) && IS_ENABLED(CONFIG_PM_DEVICE_POWER_DOMAIN)
static void nrfs_swext_driver_notify_children(const struct device *dev,
					      enum pm_device_action action)
{
	pm_device_children_action_run(dev, action, NULL);
}
#else
static void nrfs_swext_driver_notify_children(const struct device *dev,
					      enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
}
#endif

static int nrfs_swext_driver_suspend(const struct device *dev)
{
	int ret;

	nrfs_swext_driver_notify_children(dev, PM_DEVICE_ACTION_TURN_OFF);

	ret = nrfs_swext_driver_power_down(dev);
	if (ret) {
		nrfs_swext_driver_notify_children(dev, PM_DEVICE_ACTION_TURN_ON);
	}

	return ret;
}

static int nrfs_swext_driver_resume(const struct device *dev)
{
	int ret;

	ret = nrfs_swext_driver_power_up(dev);
	if (ret == 0) {
		nrfs_swext_driver_notify_children(dev, PM_DEVICE_ACTION_TURN_ON);
	}

	return ret;
}

static int nrfs_swext_driver_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = nrfs_swext_driver_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = nrfs_swext_driver_resume(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	};

	return ret;
}

static int nrfs_swext_driver_init(const struct device *dev)
{
	struct nrfs_swext_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("waiting for nrfs backend connected");
	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("nrfs backend not connected");
		return -ENODEV;
	}

	err = nrfs_swext_init(nrfs_swext_driver_evt_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to init swext service");
		return -ENODEV;
	}

	k_sem_init(&dev_data->evt_sem, 0, 1);
	return pm_device_driver_init(dev, nrfs_swext_driver_pm_action);
}

PM_DEVICE_DT_INST_DEFINE(0, nrfs_swext_driver_pm_action);

BUILD_ASSERT(DT_INST_PROP(0, max_current_ua) <= UINT16_MAX);
BUILD_ASSERT(DT_INST_PROP(0, current_limit_ua) <= DT_INST_PROP(0, max_current_ua));

static struct nrfs_swext_data data0;
static const struct nrfs_swext_config config0 = {
	.current_limit_ua = DT_INST_PROP(0, current_limit_ua),
	.enable_power_down_clamp = DT_INST_PROP(0, power_down_clamp),
};

DEVICE_DT_INST_DEFINE(
	0,
	nrfs_swext_driver_init,
	PM_DEVICE_DT_INST_GET(0),
	&data0,
	&config0,
	POST_KERNEL,
	UTIL_INC(CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO),
	NULL
);
