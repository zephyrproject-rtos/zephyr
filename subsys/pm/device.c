/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <pm/device.h>
#include <pm/device_runtime.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

const char *pm_device_state_str(enum pm_device_state state)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		return "active";
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
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOSYS;
	}

	if (pm_device_state_is_locked(dev)) {
		return -EPERM;
	}

	switch (state) {
	case PM_DEVICE_STATE_SUSPENDED:
		if (pm->state == PM_DEVICE_STATE_SUSPENDED) {
			return -EALREADY;
		} else if (pm->state == PM_DEVICE_STATE_OFF) {
			return -ENOTSUP;
		}

		action = PM_DEVICE_ACTION_SUSPEND;
		break;
	case PM_DEVICE_STATE_ACTIVE:
		if (pm->state == PM_DEVICE_STATE_ACTIVE) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_RESUME;
		break;
	case PM_DEVICE_STATE_OFF:
		if (pm->state == state) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_TURN_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	ret = pm->action_cb(dev, action);
	if (ret < 0) {
		return ret;
	}

	pm->state = state;

	return 0;
}

int pm_device_action_run(const struct device *dev,
			enum pm_device_action action)
{
	int ret;
	enum pm_device_state state;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOSYS;
	}

	switch (action) {
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
		__fallthrough;
	case PM_DEVICE_ACTION_SUSPEND:
		if (pm->state == PM_DEVICE_STATE_SUSPENDED) {
			return -EALREADY;
		} else if (pm->state == PM_DEVICE_STATE_OFF) {
			return -ENOTSUP;
		}

		state = PM_DEVICE_STATE_SUSPENDED;
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (pm->state == PM_DEVICE_STATE_ACTIVE) {
			return -EALREADY;
		}

		state = PM_DEVICE_STATE_ACTIVE;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		if (pm->state == PM_DEVICE_STATE_OFF) {
			return -EALREADY;
		}

		state = PM_DEVICE_STATE_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	ret = pm->action_cb(dev, action);
	if (ret < 0) {
		return ret;
	}

	pm->state = state;

	return 0;
}

int pm_device_state_get(const struct device *dev,
			enum pm_device_state *state)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOSYS;
	}

	*state = pm->state;

	return 0;
}

bool pm_device_is_any_busy(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	for (const struct device *dev = devs; dev < (devs + devc); dev++) {
		struct pm_device *pm = dev->pm;

		if (pm == NULL) {
			continue;
		}

		if (atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_BUSY)) {
			return true;
		}
	}

	return false;
}

bool pm_device_is_busy(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_set(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return;
	}

	atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_clear(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return;
	}

	atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

bool pm_device_wakeup_enable(struct device *dev, bool enable)
{
	atomic_val_t flags, new_flags;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	flags =	atomic_get(&pm->flags);

	if ((flags & BIT(PM_DEVICE_FLAG_WS_CAPABLE)) == 0U) {
		return false;
	}

	if (enable) {
		new_flags = flags |
			BIT(PM_DEVICE_FLAG_WS_ENABLED);
	} else {
		new_flags = flags & ~BIT(PM_DEVICE_FLAG_WS_ENABLED);
	}

	return atomic_cas(&pm->flags, flags, new_flags);
}

bool pm_device_wakeup_is_enabled(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags,
			       PM_DEVICE_FLAG_WS_ENABLED);
}

bool pm_device_wakeup_is_capable(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags,
			       PM_DEVICE_FLAG_WS_CAPABLE);
}

void pm_device_state_lock(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if ((pm != NULL) && !pm_device_runtime_is_enabled(dev)) {
		atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_STATE_LOCKED);
	}
}

void pm_device_state_unlock(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm != NULL) {
		atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_STATE_LOCKED);
	}
}

bool pm_device_state_is_locked(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags,
			       PM_DEVICE_FLAG_STATE_LOCKED);
}
