/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

static const enum pm_device_state action_target_state[] = {
	[PM_DEVICE_ACTION_SUSPEND] = PM_DEVICE_STATE_SUSPENDED,
	[PM_DEVICE_ACTION_RESUME] = PM_DEVICE_STATE_ACTIVE,
	[PM_DEVICE_ACTION_TURN_OFF] = PM_DEVICE_STATE_OFF,
	[PM_DEVICE_ACTION_TURN_ON] = PM_DEVICE_STATE_SUSPENDED,
};
static const enum pm_device_state action_expected_state[] = {
	[PM_DEVICE_ACTION_SUSPEND] = PM_DEVICE_STATE_ACTIVE,
	[PM_DEVICE_ACTION_RESUME] = PM_DEVICE_STATE_SUSPENDED,
	[PM_DEVICE_ACTION_TURN_OFF] = PM_DEVICE_STATE_SUSPENDED,
	[PM_DEVICE_ACTION_TURN_ON] = PM_DEVICE_STATE_OFF,
};

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
	struct pm_device_base *pm = dev->pm_base;
	int ret;

	if (pm == NULL) {
		return -ENOSYS;
	}

	/* Validate action against current state */
	if (pm->state == action_target_state[action]) {
		return -EALREADY;
	}
	if (pm->state != action_expected_state[action]) {
		return -ENOTSUP;
	}

	ret = pm->action_cb(dev, action);
	if (ret < 0) {
		/*
		 * TURN_ON and TURN_OFF are actions triggered by a power domain
		 * when it is resumed or suspended, which means that the energy
		 * to the device will be removed or added. For this reason, if
		 * the transition fails or the device does not handle these
		 * actions its state still needs to updated to reflect its
		 * physical behavior.
		 *
		 * The function will still return the error code so the domain
		 * can take whatever action is more appropriated.
		 */
		switch (action) {
		case PM_DEVICE_ACTION_TURN_ON:
			/* Store an error flag when the transition explicitly fails */
			if (ret != -ENOTSUP) {
				atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_TURN_ON_FAILED);
			}
			__fallthrough;
		case PM_DEVICE_ACTION_TURN_OFF:
			pm->state = action_target_state[action];
			break;
		default:
			break;
		}
		return ret;
	}

	pm->state = action_target_state[action];
	/* Power up flags are no longer relevant */
	if (action == PM_DEVICE_ACTION_TURN_OFF) {
		atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_PD_CLAIMED);
		atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_TURN_ON_FAILED);
	}

	return 0;
}

static int power_domain_add_or_remove(const struct device *dev,
				      const struct device *domain,
				      bool add)
{
#if defined(CONFIG_DEVICE_DEPS_DYNAMIC)
	device_handle_t *rv = domain->deps;
	device_handle_t dev_handle = -1;
	size_t i = 0, region = 0;

	/*
	 * Supported devices are stored as device handle and not
	 * device pointers. So, it is necessary to find what is
	 * the handle associated to the given device.
	 */
	STRUCT_SECTION_FOREACH(device, iter_dev) {
		if (iter_dev == dev) {
			dev_handle = i + 1;
			break;
		}

		i++;
	}

	/*
	 * The last part is to find an available slot in the
	 * supported section of handles array and replace it
	 * with the device handle.
	 */
	while (region != 2) {
		if (*rv == Z_DEVICE_DEPS_SEP) {
			region++;
		}
		rv++;
	}

	i = 0;
	while (rv[i] != Z_DEVICE_DEPS_ENDS) {
		if (add == false) {
			if (rv[i] == dev_handle) {
				dev->pm_base->domain = NULL;
				rv[i] = DEVICE_HANDLE_NULL;
				return 0;
			}
		} else {
			if (rv[i] == DEVICE_HANDLE_NULL) {
				dev->pm_base->domain = domain;
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

#ifdef CONFIG_DEVICE_DEPS
struct pm_visitor_context {
	pm_device_action_failed_cb_t failure_cb;
	enum pm_device_action action;
};

static int pm_device_children_visitor(const struct device *dev, void *context)
{
	struct pm_visitor_context *visitor_context = context;
	int rc;

	rc = pm_device_action_run(dev, visitor_context->action);
	if ((visitor_context->failure_cb != NULL) && (rc < 0)) {
		/* Stop the iteration if the callback requests it */
		if (!visitor_context->failure_cb(dev, rc)) {
			return rc;
		}
	}
	return 0;
}

void pm_device_children_action_run(const struct device *dev,
				   enum pm_device_action action,
				   pm_device_action_failed_cb_t failure_cb)
{
	struct pm_visitor_context visitor_context = {
		.failure_cb = failure_cb,
		.action = action
	};

	(void)device_supported_foreach(dev, pm_device_children_visitor, &visitor_context);
}
#endif

int pm_device_state_get(const struct device *dev,
			enum pm_device_state *state)
{
	struct pm_device_base *pm = dev->pm_base;

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
		struct pm_device_base *pm = dev->pm_base;

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
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_set(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return;
	}

	atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_clear(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return;
	}

	atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_BUSY);
}

bool pm_device_wakeup_enable(const struct device *dev, bool enable)
{
	atomic_val_t flags, new_flags;
	struct pm_device_base *pm = dev->pm_base;

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
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags,
			       PM_DEVICE_FLAG_WS_ENABLED);
}

bool pm_device_wakeup_is_capable(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return false;
	}

	return atomic_test_bit(&pm->flags,
			       PM_DEVICE_FLAG_WS_CAPABLE);
}

bool pm_device_on_power_domain(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	struct pm_device_base *pm = dev->pm_base;

	if (pm == NULL) {
		return false;
	}
	return pm->domain != NULL;
#else
	ARG_UNUSED(dev);
	return false;
#endif
}

bool pm_device_is_powered(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	struct pm_device_base *pm = dev->pm_base;

	/* If a device doesn't support PM or is not under a PM domain,
	 * assume it is always powered on.
	 */
	return (pm == NULL) ||
	       (pm->domain == NULL) ||
	       (pm->domain->pm_base->state == PM_DEVICE_STATE_ACTIVE);
#else
	ARG_UNUSED(dev);
	return true;
#endif
}

int pm_device_driver_init(const struct device *dev,
			  pm_device_action_cb_t action_cb)
{
	struct pm_device_base *pm = dev->pm_base;
	int rc;

	/* Work only needs to be performed if the device is powered */
	if (!pm_device_is_powered(dev)) {
		/* Start in off mode */
		pm_device_init_off(dev);
		return 0;
	}

	/* Run power-up logic */
	rc = action_cb(dev, PM_DEVICE_ACTION_TURN_ON);
	if ((rc < 0) && (rc != -ENOTSUP)) {
		return rc;
	}

	/* If device has no PM structure */
	if (pm == NULL) {
		/* Device should always be active */
		return action_cb(dev, PM_DEVICE_ACTION_RESUME);
	}

	/* If device will have PM device runtime enabled */
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) &&
	    atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_AUTO)) {
		/* Init into suspend mode.
		 * This saves a SUSPENDED->ACTIVE->SUSPENDED cycle.
		 */
		pm_device_init_suspended(dev);
		return 0;
	}

	/* Startup into active mode */
	return action_cb(dev, PM_DEVICE_ACTION_RESUME);
}
