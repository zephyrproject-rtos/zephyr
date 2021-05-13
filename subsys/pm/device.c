/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <string.h>
#include <device.h>
#include "policy/pm_policy.h"

#if defined(CONFIG_PM)
#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/* Number of devices in any suspension state. */
static size_t num_susp;
/* Device index from which resume should start */
static size_t resume_point;

static int _pm_devices(uint32_t state)
{
	const struct device *devs;
	size_t devs_cnt;

	devs_cnt = z_device_get_all_static(&devs);

	num_susp = 0U;
	resume_point = 0U;

	for (size_t i = devs_cnt; i > 0U; i--) {
		int ret;
		const struct device *dev;
		uint32_t curr_state;

		resume_point = i - 1U;
		dev = &devs[resume_point];

		/* ignore busy devices */
		if (device_busy_check(dev) != 0) {
			continue;
		}

		/* check if already in the given state (or OFF) */
		ret = pm_device_state_get(dev, &curr_state);
		if (ret == -ENOSYS) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Could not get device state (%d)", ret);
			return ret;
		}

		if ((curr_state != PM_DEVICE_STATE_OFF) && (curr_state != state)) {
			ret = pm_device_state_set(dev, state, NULL, NULL);
			if (ret < 0) {
				LOG_ERR("Could not set device state (%d)", ret);
				return ret;
			}
		}

		/* if suspended (or already), increase counter */
		num_susp++;
	}

	return 0;
}

int pm_suspend_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_SUSPEND);
}

int pm_low_power_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_LOW_POWER);
}

int pm_force_suspend_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_FORCE_SUSPEND);
}

void pm_resume_devices(void)
{
	const struct device *devs;
	size_t devs_cnt;

	devs_cnt = z_device_get_all_static(&devs);

	for (size_t i = resume_point; (i < devs_cnt) && (num_susp > 0U); i++) {
		int ret;
		const struct device *dev = &devs[i];

		ret = pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
		if (ret == -ENOSYS) {
			continue;
		}
		__ASSERT(ret == 0, "Could not set device state");

		num_susp--;
	}
}
#endif /* defined(CONFIG_PM) */

const char *pm_device_state_str(uint32_t state)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		return "active";
	case PM_DEVICE_STATE_LOW_POWER:
		return "low power";
	case PM_DEVICE_STATE_SUSPEND:
		return "suspend";
	case PM_DEVICE_STATE_FORCE_SUSPEND:
		return "force suspend";
	case PM_DEVICE_STATE_OFF:
		return "off";
	default:
		return "";
	}
}

int pm_device_state_set(const struct device *dev, uint32_t device_power_state,
			pm_device_cb cb, void *arg)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	return dev->pm_control(dev, PM_DEVICE_STATE_SET,
			       &device_power_state, cb, arg);
}

int pm_device_state_get(const struct device *dev, uint32_t *device_power_state)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	return dev->pm_control(dev, PM_DEVICE_STATE_GET,
			       device_power_state, NULL, NULL);
}
