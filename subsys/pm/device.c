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

/** Start of devices supporting power management. */
extern const struct device *__pm_device_start[];
/** End of devices supporting power management. */
extern const struct device *__pm_device_end[];
/** Number of suspended devices. */
static size_t pm_device_susp;

static bool should_suspend(const struct device *dev, uint32_t state)
{
	int rc;
	uint32_t current_state;

	if (device_busy_check(dev) != 0) {
		return false;
	}

	rc = pm_device_state_get(dev, &current_state);
	if ((rc != -ENOTSUP) && (rc != 0)) {
		LOG_DBG("Was not possible to get device %s state: %d",
			dev->name, rc);
		return true;
	}

	/*
	 * If the device is currently powered off or the request was
	 * to go to the same state, just ignore it.
	 */
	if ((current_state == PM_DEVICE_STATE_OFF) ||
			(current_state == state)) {
		return false;
	}

	return true;
}

static int _pm_devices(uint32_t state)
{
	size_t pm_device_cnt = __pm_device_end - __pm_device_start;

	pm_device_susp = 0U;

	for (size_t i = pm_device_cnt - 1U; i >= 0U; i--) {
		const struct device *dev = __pm_device_start[i];
		bool suspend;
		int rc;

		suspend = should_suspend(dev, state);
		if (suspend) {
			/*
			 * Don't bother the device if it is currently
			 * in the right state.
			 */
			rc = pm_device_state_set(dev, state, NULL, NULL);
			if ((rc != -ENOTSUP) && (rc != 0)) {
				LOG_DBG("%s did not enter %s state: %d",
					dev->name, pm_device_state_str(state),
					rc);
				return rc;
			}

			/*
			 * Just mark as suspended devices that were suspended now
			 * otherwise we will resume devices that were already suspended
			 * and not being used.
			 * This still not optimal, since we are not distinguishing
			 * between other states like DEVICE_PM_LOW_POWER_STATE.
			 */
			++pm_device_susp;
		}
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
	size_t pm_device_cnt = __pm_device_end - __pm_device_start;
	size_t i = pm_device_cnt - pm_device_susp;

	pm_device_susp = 0U;
	while (i < pm_device_cnt) {
		pm_device_state_set(__pm_device_start[i],
				    PM_DEVICE_STATE_ACTIVE, NULL, NULL);
		++i;
	}
}
#endif /* defined(CONFIG_PM) */

int pm_device_none(const struct device *dev, uint32_t ctrl_command,
		   uint32_t *context, pm_device_cb cb, void *arg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ctrl_command);
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(arg);

	return -ENOSYS;
}

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
	return dev->pm_control(dev, PM_DEVICE_STATE_SET,
			       &device_power_state, cb, arg);
}

int pm_device_state_get(const struct device *dev, uint32_t *device_power_state)
{
	return dev->pm_control(dev, PM_DEVICE_STATE_GET,
			       device_power_state, NULL, NULL);
}
