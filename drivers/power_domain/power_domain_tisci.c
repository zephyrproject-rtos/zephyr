/*
 * Copyright 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/drivers/power/ti,sci_pm_domain.h>
LOG_MODULE_REGISTER(tisci_pd);

#define DT_DRV_COMPAT ti_sci_pm_domain

const struct device *dmsc;

struct power_domain {
	uint32_t devid;
	uint32_t mode;
};

static int tisci_power_domain_on(struct power_domain *pd)
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

static int tisci_power_domain_off(struct power_domain *pd)
{
	int ret = tisci_cmd_put_device(dmsc, pd->devid);

	if (ret) {
		LOG_ERR("TISCI PM: put_device(%u) failed (%d)\n", pd->devid, ret);
	}

	return ret;
}

static int tisci_pd_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct power_domain *data = dev->data;

	LOG_DBG("TISCI PM action %d on devid %d, mode %d", action, data->devid, data->mode);
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return 0;
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = tisci_power_domain_on(data);
		return ret;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = tisci_power_domain_off(data);
		return ret;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int tisci_pd_init(const struct device *dev)
{
	dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));
	if (dmsc == NULL) {
		LOG_ERR("DMSC device not found");
		return -ENODEV;
	}
	return pm_device_runtime_enable(dev);
}

#define TISCI_PD_DEVICE_DEFINE(inst)                                                               \
	static struct power_domain power_domain_data_##inst = {                                    \
		.devid = DT_INST_PROP(inst, tisci_device_id),                                      \
		.mode = DT_INST_PROP(inst, tisci_device_mode),                                     \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, tisci_pd_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(inst, tisci_pd_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &power_domain_data_##inst, NULL, PRE_KERNEL_1,                       \
			      CONFIG_POWER_DOMAIN_TISCI_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TISCI_PD_DEVICE_DEFINE);
