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

#if defined(CONFIG_PM_DEVICE)
extern const struct device *__pm_device_slots_start[];

/* Number of devices successfully suspended. */
static size_t num_susp;

static int _pm_devices(enum pm_device_state state)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		/* ignore busy devices */
		if (pm_device_is_busy(dev) || pm_device_wakeup_is_enabled(dev)) {
			continue;
		}

		ret = pm_device_state_set(dev, state);
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Device %s did not enter %s state (%d)",
				dev->name, pm_device_state_str(state), ret);
			return ret;
		}

		__pm_device_slots_start[num_susp] = dev;
		num_susp++;
	}

	return 0;
}

int pm_suspend_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_SUSPENDED);
}

int pm_low_power_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_LOW_POWER);
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
#endif /* defined(CONFIG_PM_DEVICE) */

const char *pm_device_state_str(enum pm_device_state state)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		return "active";
	case PM_DEVICE_STATE_LOW_POWER:
		return "low power";
	case PM_DEVICE_STATE_SUSPENDED:
		return "suspended";
	case PM_DEVICE_STATE_OFF:
		return "off";
	default:
		return "";
	}
}

int pm_device_state_set(const struct device *dev,
			enum pm_device_state state)
{
	int ret;
	enum pm_device_action action;

	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	if (atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_TRANSITIONING)) {
		return -EBUSY;
	}

	switch (state) {
	case PM_DEVICE_STATE_SUSPENDED:
		if (dev->pm->state == PM_DEVICE_STATE_SUSPENDED) {
			return -EALREADY;
		} else if (dev->pm->state == PM_DEVICE_STATE_OFF) {
			return -ENOTSUP;
		}

		action = PM_DEVICE_ACTION_SUSPEND;
		break;
	case PM_DEVICE_STATE_ACTIVE:
		if (dev->pm->state == PM_DEVICE_STATE_ACTIVE) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_RESUME;
		break;
	case PM_DEVICE_STATE_LOW_POWER:
		if (dev->pm->state == state) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_LOW_POWER;
		break;
	case PM_DEVICE_STATE_OFF:
		if (dev->pm->state == state) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_TURN_OFF;
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
