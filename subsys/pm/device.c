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

#if defined(CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC)
int pm_device_power_domain_remove(const struct device *dev,
				  const struct device *domain)
{
	uint16_t i;

	if (dev->pm_base == NULL || domain->pm_base == NULL) {
		return -ENOTSUP;
	}

	if (dev->pm_base->domain != domain) {
		return -ENOENT;
	}

	for (i = 0; i < domain->pm_base->domain_children_size; i++) {
		if (domain->pm_base->domain_children[i] == dev) {
			domain->pm_base->domain_children[i] = NULL;
			break;
		}
	}

	dev->pm_base->domain = NULL;
	return 0;
}

int pm_device_power_domain_add(const struct device *dev,
			       const struct device *domain)
{
	uint16_t i;

	if (dev->pm_base == NULL || domain->pm_base == NULL) {
		return -ENOTSUP;
	}

	if (dev->pm_base->domain != NULL) {
		return -EALREADY;
	}

	for (i = 0; i < domain->pm_base->domain_children_size; i++) {
		if (domain->pm_base->domain_children[i] == NULL) {
			domain->pm_base->domain_children[i] = dev;
			break;
		}
	}

	if (i == domain->pm_base->domain_children_size) {
		return -ENOMEM;
	}

	dev->pm_base->domain = domain;
	return 0;
}

#else

int pm_device_power_domain_remove(const struct device *dev,
				  const struct device *domain)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(domain);
	return -ENOSYS;
}

int pm_device_power_domain_add(const struct device *dev,
			       const struct device *domain)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(domain);
	return -ENOSYS;
}
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC */

#if defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
void pm_device_children_action_run(const struct device *dev,
				   enum pm_device_action action,
				   pm_device_action_failed_cb_t failure_cb)
{
	struct pm_device_base *domain_pm_base;
	const struct device *const *domain_children;
	uint16_t domain_children_size;
	const struct device *domain_child;
	int ret;

	domain_pm_base = dev->pm_base;

	if (domain_pm_base == NULL) {
		return;
	}

	domain_children = domain_pm_base->domain_children;
	domain_children_size = domain_pm_base->domain_children_size;

	for (uint16_t i = 0; i < domain_children_size; i++) {
		domain_child = domain_children[i];

		if (domain_child == NULL) {
			continue;
		}

		ret = pm_device_action_run(domain_child, action);
		if (ret == 0 || failure_cb == NULL) {
			continue;
		}

		if (!failure_cb(domain_child, ret)) {
			return;
		}
	}
}
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */

#if defined(CONFIG_PM_DEVICE_POWER_DOMAIN)
static void pm_device_domain_children_turn_onoff(const struct device *dev, bool on)
{
	struct pm_device_base *pm_base = dev->pm_base;
	const struct device *child;
	const enum pm_device_action action = on
					   ? PM_DEVICE_ACTION_TURN_ON
					   : PM_DEVICE_ACTION_TURN_OFF;
	const char *const action_str = on
				     ? "turned on"
				     : "turned off";

	for (uint16_t i = 0; i < pm_base->domain_children_size; i++) {
		child = pm_base->domain_children[i];

		if (child == NULL) {
			continue;
		}

		if (pm_device_action_run(child, action) == 0) {
			continue;
		}

		LOG_ERR("%s could not be %s", child->name, action_str);
	}
}

void pm_device_domain_children_turn_on(const struct device *dev)
{
	pm_device_domain_children_turn_onoff(dev, true);
}

void pm_device_domain_children_turn_off(const struct device *dev)
{
	pm_device_domain_children_turn_onoff(dev, false);
}
#else
void pm_device_domain_children_turn_on(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void pm_device_domain_children_turn_off(const struct device *dev)
{
	ARG_UNUSED(dev);
}
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */

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

	/* Device is currently in the OFF state */
	if (pm) {
		pm->state = PM_DEVICE_STATE_OFF;
	}

	/* Work only needs to be performed if the device is powered */
	if (!pm_device_is_powered(dev)) {
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

	/* Device is currently in the SUSPENDED state */
	pm->state = PM_DEVICE_STATE_SUSPENDED;

	/* If device will have PM device runtime enabled */
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) &&
	    atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_AUTO)) {
		return 0;
	}

	/* Startup into active mode */
	rc = action_cb(dev, PM_DEVICE_ACTION_RESUME);

	/* Device is now in the ACTIVE state */
	pm->state = PM_DEVICE_STATE_ACTIVE;

	/* Return the PM_DEVICE_ACTION_RESUME result */
	return rc;
}
