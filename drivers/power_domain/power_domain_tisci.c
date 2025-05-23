/*
 * Copyright 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/firmware/tisci/ti_sci.h>
#include <zephyr/drivers/power/ti,sci_pm_domain.h>
LOG_MODULE_REGISTER(tisci_pd);

#define DT_DRV_COMPAT ti_sci_pm_domain

const struct device *dmsc;

struct power_domain {
	uint32_t devid;
	uint32_t mode;
};

static int ti_sci_power_domain_on(struct power_domain *pd)
{
	int ret;

	if (TI_SCI_PD_EXCLUSIVE) {
		ret = ti_sci_cmd_get_device_exclusive(dmsc, pd->devid);
	} else {
		ret = ti_sci_cmd_get_device(dmsc, pd->devid);
	}

	if (ret) {
		LOG_ERR("TISCI PM: get_device(%u) failed (%d)\n", pd->devid, ret);
	}

	return ret;
}

static int ti_sci_power_domain_off(struct power_domain *pd)
{
	int ret = ti_sci_cmd_put_device(dmsc, pd->devid);

	if (ret) {
		LOG_ERR("TISCI PM: put_device(%u) failed (%d)\n", pd->devid, ret);
	}

	return ret;
}

static int tisci_pd_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct power_domain *data = dev->data;

	LOG_DBG("TISCI PM action %d on devid %d, mode %d", action, data->devid, data->mode);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return 0;
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		ti_sci_power_domain_on(data);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ti_sci_power_domain_off(data);
		break;
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
		.devid = DT_INST_PROP(inst, ti_sci_device_id),                                     \
		.mode = DT_INST_PROP(inst, ti_sci_device_mode),                                    \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, tisci_pd_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(inst, tisci_pd_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &power_domain_data_##inst, NULL, PRE_KERNEL_1,                       \
			      CONFIG_POWER_DOMAIN_TISCI_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TISCI_PD_DEVICE_DEFINE);
