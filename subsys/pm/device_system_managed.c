/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

#define DT_PM_DEVICE_ENABLED(node_id)					\
	COND_CODE_1(DT_PROP(node_id, zephyr_pm_device_disabled),	\
		(), (1 +))

#define DT_PM_DEVICE_NEEDED			\
	(DT_FOREACH_STATUS_OKAY(zephyr_power_state, DT_PM_DEVICE_ENABLED) 0)

#if DT_PM_DEVICE_NEEDED
TYPE_SECTION_START_EXTERN(const struct device *, pm_device_slots);

/* Number of devices successfully suspended. */
static size_t num_susp;

bool pm_suspend_devices(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		/*
		 * Ignore uninitialized devices, busy devices, wake up sources, and
		 * devices with runtime PM enabled.
		 */
		if (!device_is_ready(dev) || pm_device_is_busy(dev) ||
		    pm_device_wakeup_is_enabled(dev) ||
		    pm_device_runtime_is_enabled(dev)) {
			continue;
		}

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Device %s did not enter %s state (%d)",
				dev->name,
				pm_device_state_str(PM_DEVICE_STATE_SUSPENDED),
				ret);
			return false;
		}

		TYPE_SECTION_START(pm_device_slots)[num_susp] = dev;
		num_susp++;
	}

	return true;
}

void pm_resume_devices(void)
{
	for (int i = (num_susp - 1); i >= 0; i--) {
		pm_device_action_run(TYPE_SECTION_START(pm_device_slots)[i],
				    PM_DEVICE_ACTION_RESUME);
	}

	num_susp = 0;
}

#else /* !DT_PM_DEVICE_NEEDED */

void pm_resume_devices(void)
{
}

bool pm_suspend_devices(void)
{
	return true;
}

#endif
