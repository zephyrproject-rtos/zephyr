/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define DT_DRV_COMPAT silabs_siwx91x_power_domain

static int siwx91x_pd_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int siwx91x_pd_init(const struct device *dev)
{
	return pm_device_driver_init(dev, siwx91x_pd_pm_action);
}

#define SIWX91X_PD_INIT(inst)                                                                      \
	PM_DEVICE_DT_INST_DEFINE(inst, siwx91x_pd_pm_action);                                      \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_pd_init, PM_DEVICE_DT_INST_GET(inst), NULL, NULL,      \
			      PRE_KERNEL_1, CONFIG_SIWX91X_POWER_DOMAIN_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_PD_INIT)
