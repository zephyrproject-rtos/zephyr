/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

static bool dont_sleep;

static int dummy_open(const struct device *dev)
{
	dont_sleep = false;
	return pm_device_get(dev);
}

static int dummy_close(const struct device *dev)
{
	dont_sleep = false;
	return pm_device_put(dev);
}

static int dummy_refuse_to_sleep(const struct device *dev)
{
	dont_sleep = true;
	return 0;
}

static int dummy_device_pm_ctrl(const struct device *dev,
				enum pm_device_action action)
{
	/* dont_sleep is used to prevent pm_system_suspend() from entering
	 * PM_STATE_RUNTIME_IDLE or PM_STATE_SUSPEND_TO_RAM by
	 * pm_suspend_devices()
	 */
	if (dont_sleep) {
		if (action != PM_DEVICE_ACTION_RESUME) {
			return -EIO;
		}
	}

	return 0;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.close = dummy_close,
	.refuse_to_sleep = dummy_refuse_to_sleep,
};

int dummy_init(const struct device *dev)
{
	pm_device_enable(dev);
	return 0;
}

DEVICE_DEFINE(dummy_device, DUMMY_NAME, &dummy_init,
		dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);

DEVICE_DEFINE(dummy_device_no_pm_control, DUMMY_NO_PM, &dummy_init,
		NULL, NULL, NULL, APPLICATION,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

static int dummy_wakeup_dev_pm_ctrl(const struct device *dev,
				enum pm_device_action action)
{
	return 0;
}

int dummy_wakeup_dev_init(const struct device *dev)
{
	return 0;
}

/* for wakeup capable device. bit PM_DEVICE_FLAGS_WS_ENABLED of pm->flags
 * is set when device is defined, and there is no interface to set/clear it
 */
DEVICE_DT_DEFINE(DT_INST(0, zephyr_wakeup_dev), &dummy_wakeup_dev_init,
		dummy_wakeup_dev_pm_ctrl, NULL, NULL,
		APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

static int dummy_pk_close(const struct device *dev)
{
	return pm_device_put(dev);
}

static int dummy_device_pk_pm_ctrl(const struct device *dev,
				enum pm_device_action action)
{
	return 0;
}

static const struct dummy_driver_api pk_funcs = {
	.close = dummy_pk_close,
};

static int dummy_pre_kernel_init(const struct device *dev)
{
	pm_device_enable(dev);
	pm_device_get(dev);
	return 0;
}

static int pk_device_close(const struct device *unused)
{
	ARG_UNUSED(unused);

	const struct device *pk_dev;
	struct dummy_driver_api *api;

	pk_dev = device_get_binding(DUMMY_PK_NAME);
	api = (struct dummy_driver_api *)pk_dev->api;
	api->close(pk_dev);
	return 0;
}

/* for k_is_pre_kernel(), PRE_KERNEL_1, PRE_KERENEL_2 */
SYS_INIT(pk_device_close, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
DEVICE_DEFINE(dummy_pre_kernel_device, DUMMY_PK_NAME, &dummy_pre_kernel_init,
		dummy_device_pk_pm_ctrl, NULL, NULL, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &pk_funcs);
DEVICE_DEFINE(dummy_pre_kernel_device_no_pm_ctrl, DUMMY_PK_NAME,
		&dummy_pre_kernel_init, NULL, NULL, NULL, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &pk_funcs);
