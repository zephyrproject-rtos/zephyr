/*
 * Copyright (c) 2026 Rodrigo Peixoto <rodrigopex@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_power_domain_log

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(power_domain_log, LOG_LEVEL_INF);

static int pd_log_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("%s: resumed", dev->name);
		/* Power reached this node: hand it down to the dependents. */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Power is about to be cut: tell the dependents first. */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		LOG_DBG("%s: suspended", dev->name);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		LOG_DBG("%s: on", dev->name);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		LOG_DBG("%s: off", dev->name);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pd_log_init(const struct device *dev)
{
	return pm_device_driver_init(dev, pd_log_action);
}

#define PD_LOG_DEFINE(inst)						       \
	PM_DEVICE_DT_INST_DEFINE(inst, pd_log_action);			       \
	DEVICE_DT_INST_DEFINE(inst, pd_log_init, PM_DEVICE_DT_INST_GET(inst),   \
			      NULL, NULL, POST_KERNEL,			       \
			      CONFIG_APPLICATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PD_LOG_DEFINE)
