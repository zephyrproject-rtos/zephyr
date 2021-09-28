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

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

const char *pm_device_state_str(enum pm_device_state state)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		return "active";
	case PM_DEVICE_STATE_SUSPENDED:
		return "suspended";
	default:
		return "";
	}
}

int pm_device_action_run(const struct device *dev,
			enum pm_device_action action)
{
	int ret;
	enum pm_device_state state;

	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	if (atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_TRANSITIONING)) {
		return -EBUSY;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (dev->pm->state == PM_DEVICE_STATE_SUSPENDED) {
			return -EALREADY;
		}

		state = PM_DEVICE_STATE_SUSPENDED;
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (dev->pm->state == PM_DEVICE_STATE_ACTIVE) {
			return -EALREADY;
		}

		state = PM_DEVICE_STATE_ACTIVE;
		break;
	default:
		return -ENOTSUP;
	}

	ret = dev->pm_control(dev, action);
	if (ret < 0) {
		return ret;
	}

	dev->pm->state = state;

	return 0;
}

int pm_device_state_get(const struct device *dev,
			enum pm_device_state *state)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	*state = dev->pm->state;

	return 0;
}

bool pm_device_is_any_busy(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	for (const struct device *dev = devs; dev < (devs + devc); dev++) {
		if (atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY)) {
			return true;
		}
	}

	return false;
}

bool pm_device_is_busy(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_set(const struct device *dev)
{
	atomic_set_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_clear(const struct device *dev)
{
	atomic_clear_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

bool pm_device_wakeup_enable(struct device *dev, bool enable)
{
	atomic_val_t flags, new_flags;

	flags =	 atomic_get(&dev->pm->flags);

	if ((flags & BIT(PM_DEVICE_FLAGS_WS_CAPABLE)) == 0U) {
		return false;
	}

	if (enable) {
		new_flags = flags |
			BIT(PM_DEVICE_FLAGS_WS_ENABLED);
	} else {
		new_flags = flags & ~BIT(PM_DEVICE_FLAGS_WS_ENABLED);
	}

	return atomic_cas(&dev->pm->flags, flags, new_flags);
}

bool pm_device_wakeup_is_enabled(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags,
			       PM_DEVICE_FLAGS_WS_ENABLED);
}

bool pm_device_wakeup_is_capable(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags,
			       PM_DEVICE_FLAGS_WS_CAPABLE);
}
