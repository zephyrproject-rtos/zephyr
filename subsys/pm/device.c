/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <string.h>
#include <device.h>
#include <pm/policy.h>

#if defined(CONFIG_PM)
#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

extern const struct device __device_start[];
extern const struct device __device_end[];

extern const struct device *__pm_device_slots_start[];

/* Number of devices successfully suspended. */
static size_t num_susp;

static bool should_suspend(const struct device *dev, enum pm_device_state state)
{
	int rc;
	enum pm_device_state current_state;

	if (device_busy_check(dev) != 0) {
		return false;
	}

	rc = pm_device_state_get(dev, &current_state);
	if ((rc == -ENOSYS) || (rc != 0)) {
		LOG_DBG("Was not possible to get device %s state: %d",
			dev->name, rc);
		return false;
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
	const struct device *dev;
	num_susp = 0;

	for (dev = (__device_end - 1); dev > __device_start; dev--) {
		bool suspend;
		int rc;

		suspend = should_suspend(dev, state);
		if (suspend) {
			/*
			 * Don't bother the device if it is currently
			 * in the right state.
			 */
			rc = pm_device_state_set(dev, state);
			if ((rc != -ENOSYS) && (rc != 0)) {
				LOG_DBG("%s did not enter %s state: %d",
					dev->name, pm_device_state_str(state),
					rc);
				return rc;
			}

			__pm_device_slots_start[num_susp] = dev;
			num_susp++;
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
	size_t i;

	for (i = 0; i < num_susp; i++) {
		pm_device_state_set(__pm_device_slots_start[i],
				    PM_DEVICE_STATE_ACTIVE);
	}

	num_susp = 0;
}
#endif /* defined(CONFIG_PM) */

const char *pm_device_state_str(enum pm_device_state state)
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

int pm_device_state_set(const struct device *dev,
			enum pm_device_state device_power_state)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	return dev->pm_control(dev, PM_DEVICE_STATE_SET,
			       &device_power_state);
}

int pm_device_state_get(const struct device *dev,
			enum pm_device_state *device_power_state)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	return dev->pm_control(dev, PM_DEVICE_STATE_GET,
			       device_power_state);
}
