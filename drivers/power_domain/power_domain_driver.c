/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT power_domain

static int power_domain_driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	return 0;
}

static int power_domain_driver_init(const struct device *dev)
{
	return pm_device_driver_init(dev, power_domain_driver_pm_action);
}

#define POWER_DOMAIN_DEVICE(inst)								\
	PM_DEVICE_DT_INST_DEFINE(inst, power_domain_driver_pm_action);				\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		power_domain_driver_init,							\
		PM_DEVICE_DT_INST_GET(inst),							\
		NULL,										\
		NULL,										\
		PRE_KERNEL_1,									\
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,						\
		NULL										\
	);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
