/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr/types.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

static int dummy_open(const struct device *dev)
{
	return pm_device_get(dev);
}

static int dummy_close(const struct device *dev)
{
	return pm_device_put(dev);
}

static int dummy_device_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				enum pm_device_state *state)
{
	int ret;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		ret = 0;
		break;
	default:
		ret = -EINVAL;

	}

	return ret;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.close = dummy_close,
};

int dummy_init(const struct device *dev)
{
	pm_device_enable(dev);
	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
