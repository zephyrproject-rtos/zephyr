/*
 * Copyright 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
LOG_MODULE_REGISTER(tisci_pd);

#define DT_DRV_COMPAT ti_sci_pm_domain

const struct device *dmsc = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(dmsc));

struct power_domain {
	uint32_t devid;
	bool mode;
};

static int tisci_power_domain_on(const struct power_domain *pd)
{
	int ret;

	if (pd->mode) {
		ret = tisci_cmd_get_device_exclusive(dmsc, pd->devid);
	} else {
		ret = tisci_cmd_get_device(dmsc, pd->devid);
	}

	if (ret) {
		LOG_ERR("TISCI PM: get_device(%u) failed (%d)\n", pd->devid, ret);
	}

	return ret;
}

static int tisci_power_domain_off(const struct power_domain *pd)
{
	int ret = tisci_cmd_put_device(dmsc, pd->devid);

	if (ret) {
		LOG_ERR("TISCI PM: put_device(%u) failed (%d)\n", pd->devid, ret);
	}

	return ret;
}

static int tisci_pd_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct power_domain *data = dev->config;

	LOG_DBG("TISCI PM action %d on devid %d, mode %d", action, data->devid, data->mode);
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = tisci_power_domain_on(data);
		return ret;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = tisci_power_domain_off(data);
		return ret;
	case PM_DEVICE_ACTION_TURN_ON:
		return 0;
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int tisci_pd_init(const struct device *dev)
{
	int ret;

	if (dmsc == NULL) {
		LOG_ERR("DMSC device not found");
		return -ENODEV;
	}

	ret = pm_device_driver_init(dev, tisci_pd_pm_action);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime PM: %d", ret);
		return ret;
	}

	return 0;
}

#define TISCI_PD_DEVICE_DEFINE(inst)                                                               \
	static struct power_domain power_domain_data_##inst = {                                    \
		.devid = DT_INST_PROP(inst, tisci_device_id),                                      \
		.mode = DT_INST_ENUM_IDX(inst, tisci_device_mode),                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, tisci_pd_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(inst, tisci_pd_init, PM_DEVICE_DT_INST_GET(inst), NULL,              \
			      &power_domain_data_##inst, PRE_KERNEL_1,                             \
			      CONFIG_POWER_DOMAIN_TISCI_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TISCI_PD_DEVICE_DEFINE);
