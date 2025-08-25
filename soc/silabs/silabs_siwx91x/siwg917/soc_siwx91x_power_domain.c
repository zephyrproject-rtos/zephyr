/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define SI91X_POWER_DOMAIN DT_NODELABEL(siwx91x_soc_pd)

static int domain_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

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
		ret = -ENOTSUP;
	}

	return ret;
}

PM_DEVICE_DT_DEFINE(SI91X_POWER_DOMAIN, domain_pm_action);
DEVICE_DT_DEFINE(SI91X_POWER_DOMAIN, NULL, PM_DEVICE_DT_GET(SI91X_POWER_DOMAIN), NULL, NULL,
		 PRE_KERNEL_1, 10, NULL);
