/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
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

int pm_device_action_run(const struct device *dev,
			 enum pm_device_action action)
{
	int ret;
	enum pm_device_state state;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOSYS;
	}

	if (pm_device_state_is_locked(dev)) {
		return -EPERM;
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
	case PM_DEVICE_ACTION_TURN_ON:
		if (pm->state != PM_DEVICE_STATE_OFF) {
			return -ENOTSUP;
		}

		state = PM_DEVICE_STATE_SUSPENDED;
		break;
	default:
		return -ENOTSUP;
	}

	ret = pm->action_cb(dev, action);
	if (ret < 0) {
		/*
		 * TURN_ON and TURN_OFF are actions triggered by a power domain
		 * when it is resumed or suspended, which means that the energy
		 * to the device will be removed or added. For this reason, even
		 * if the device does not handle these actions its state needs to
		 * updated to reflect its physical behavior.
		 *
		 * The function will still return the error code so the domain
		 * can take whatever action is more appropriated.
		 */
		if ((ret == -ENOTSUP) && ((action == PM_DEVICE_ACTION_TURN_ON)
				  || (action == PM_DEVICE_ACTION_TURN_OFF))) {
			pm->state = state;
		}
		return ret;
	}

	pm->state = state;

	return 0;
}

static int power_domain_add_or_remove(const struct device *dev,
				      const struct device *domain,
				      bool add)
{
#if defined(CONFIG_HAS_DYNAMIC_DEVICE_HANDLES)
	device_handle_t *rv = domain->handles;
	device_handle_t dev_handle = -1;
	extern const struct device __device_start[];
	extern const struct device __device_end[];
	size_t i, region = 0;
	size_t numdev = __device_end - __device_start;

	/*
	 * Supported devices are stored as device handle and not
	 * device pointers. So, it is necessary to find what is
	 * the handle associated to the given device.
	 */
	for (i = 0; i < numdev; i++) {
		if (&__device_start[i] == dev) {
			dev_handle = i + 1;
			break;
		}
	}

	/*
	 * The last part is to find an available slot in the
	 * supported section of handles array and replace it
	 * with the device handle.
	 */
	while (region != 2) {
		if (*rv == DEVICE_HANDLE_SEP) {
			region++;
		}
		rv++;
	}

	i = 0;
	while (rv[i] != DEVICE_HANDLE_ENDS) {
		if (add == false) {
			if (rv[i] == dev_handle) {
				dev->pm->domain = NULL;
				rv[i] = DEVICE_HANDLE_NULL;
				return 0;
			}
		} else {
			if (rv[i] == DEVICE_HANDLE_NULL) {
				dev->pm->domain = domain;
				rv[i] = dev_handle;
				return 0;
			}
		}
		++i;
	}

	return add ? -ENOSPC : -ENOENT;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(domain);
	ARG_UNUSED(add);

	return -ENOSYS;
#endif
}

int pm_device_power_domain_remove(const struct device *dev,
				  const struct device *domain)
{
	return power_domain_add_or_remove(dev, domain, false);
}

int pm_device_power_domain_add(const struct device *dev,
			       const struct device *domain)
{
	return power_domain_add_or_remove(dev, domain, true);
}

void pm_device_children_action_run(const struct device *dev,
				   enum pm_device_action action,
				   pm_device_action_failed_cb_t failure_cb)
{
	const device_handle_t *handles;
	size_t handle_count = 0U;
	int rc = 0;

	/*
	 * We don't use device_supported_foreach here because we don't want the
	 * early exit behaviour of that function. Even if the N'th device fails
	 * to PM_DEVICE_ACTION_TURN_ON for example, we still want to run the
	 * action on the N+1'th device.
	 */
	handles = device_supported_handles_get(dev, &handle_count);

	for (size_t i = 0U; i < handle_count; ++i) {
		device_handle_t dh = handles[i];
		const struct device *cdev = device_from_handle(dh);

		if (cdev == NULL) {
			continue;
		}

		rc = pm_device_action_run(cdev, action);
		if ((failure_cb != NULL) && (rc < 0)) {
			/* Stop the iteration if the callback requests it */
			if (!failure_cb(cdev, rc)) {
				break;
			}
		}
	}
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

bool pm_device_wakeup_enable(const struct device *dev, bool enable)
{
	atomic_val_t flags, new_flags;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}

	flags = atomic_get(&pm->flags);

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

bool pm_device_on_power_domain(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return false;
	}
	return pm->domain != NULL;
#else
	return false;
#endif
}

bool pm_device_is_powered(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	struct pm_device *pm = dev->pm;

	/* If a device doesn't support PM or is not under a PM domain,
	 * assume it is always powered on.
	 */
	return (pm == NULL) ||
	       (pm->domain == NULL) ||
	       (pm->domain->pm->state == PM_DEVICE_STATE_ACTIVE);
#else
	return true;
#endif
}
