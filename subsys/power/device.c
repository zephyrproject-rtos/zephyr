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

#if defined(CONFIG_SYS_POWER_MANAGEMENT)
#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/*
 * FIXME: Remove the conditional inclusion of
 * core_devices array once we enble the capability
 * to build the device list based on devices power
 * and clock domain dependencies.
 */
#if defined(CONFIG_SOC_FAMILY_NRF)
#define MAX_PM_DEVICES	15
static const char *const core_devices[] = {
	"CLOCK",
	"sys_clock",
	"UART_0",
};
#elif defined(CONFIG_SOC_SERIES_CC13X2_CC26X2)
#define MAX_PM_DEVICES	15
static const char *const core_devices[] = {
	"sys_clock",
	"UART_0",
};
#elif defined(CONFIG_SOC_SERIES_KINETIS_K6X)
#define MAX_PM_DEVICES		1
static const char *const core_devices[] = {
	DT_LABEL(DT_INST(0, nxp_kinetis_ethernet)),
};
#elif defined(CONFIG_NET_TEST)
#define MAX_PM_DEVICES		1
static const char *const core_devices[] = {
	"",
};
#elif defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32WBX)
#define MAX_PM_DEVICES	1
static const char *const core_devices[] = {
	"sys_clock",
};
#else
#error "Add SoC's core devices list for PM"
#endif

/* Ordinal of sufficient size to index available devices. */
typedef uint16_t device_idx_t;

/* The maximum value representable with a device_idx_t. */
#define DEVICE_IDX_MAX ((device_idx_t)(-1))

/* An array of all devices in the application. */
static struct device *all_devices;

/* Indexes into all_devices for devices that support pm,
 * in dependency order (later may depend on earlier).
 */
static device_idx_t pm_devices[MAX_PM_DEVICES];

/* Number of devices that support pm */
static device_idx_t num_pm;

/* Number of devices successfully suspended. */
static device_idx_t num_susp;

const char *device_pm_state_str(uint32_t state)
{
	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		return "active";
	case DEVICE_PM_LOW_POWER_STATE:
		return "low power";
	case DEVICE_PM_SUSPEND_STATE:
		return "suspend";
	case DEVICE_PM_FORCE_SUSPEND_STATE:
		return "force suspend";
	case DEVICE_PM_OFF_STATE:
		return "off";
	default:
		return "";
	}
}

static int _sys_pm_devices(uint32_t state)
{
	num_susp = 0;

	for (int i = num_pm - 1; i >= 0; i--) {
		device_idx_t idx = pm_devices[i];
		struct device *dev = &all_devices[idx];
		int rc;

		/* TODO: Improve the logic by checking device status
		 * and set the device states accordingly.
		 */
		rc = device_set_power_state(dev, state, NULL, NULL);
		if ((rc != -ENOTSUP) && (rc != 0)) {
			LOG_DBG("%s did not enter %s state: %d",
				dev->name, device_pm_state_str(state), rc);
			return rc;
		}

		++num_susp;
	}

	return 0;
}

int sys_pm_suspend_devices(void)
{
	return _sys_pm_devices(DEVICE_PM_SUSPEND_STATE);
}

int sys_pm_low_power_devices(void)
{
	return _sys_pm_devices(DEVICE_PM_LOW_POWER_STATE);
}

int sys_pm_force_suspend_devices(void)
{
	return _sys_pm_devices(DEVICE_PM_FORCE_SUSPEND_STATE);
}

void sys_pm_resume_devices(void)
{
	device_idx_t pmi = num_pm - num_susp;

	num_susp = 0;
	while (pmi < num_pm) {
		device_idx_t idx = pm_devices[pmi];

		device_set_power_state(&all_devices[idx],
				       DEVICE_PM_ACTIVE_STATE,
				       NULL, NULL);
		++pmi;
	}
}

void sys_pm_create_device_list(void)
{
	size_t count = z_device_get_all_static(&all_devices);
	device_idx_t pmi;

	/*
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 */

	__ASSERT_NO_MSG(count <= DEVICE_IDX_MAX);

	/* Reserve initial slots for core devices. */
	num_pm = ARRAY_SIZE(core_devices);

	for (pmi = 0; (pmi < count) && (num_pm < MAX_PM_DEVICES); pmi++) {
		device_idx_t cdi = 0;
		const struct device *dev = &all_devices[pmi];

		/* Ignore "device"s that don't support PM */
		if (dev->device_pm_control == device_pm_control_nop) {
			continue;
		}

		/* Check if the device is a core device, which has a
		 * reserved slot.
		 */
		while (cdi < ARRAY_SIZE(core_devices)) {
			if (strcmp(dev->name, core_devices[cdi]) == 0) {
				pm_devices[cdi] = pmi;
				break;
			}
			++cdi;
		}

		/* Append the device if it doesn't have a reserved slot. */
		if (cdi == ARRAY_SIZE(core_devices)) {
			pm_devices[num_pm++] = pmi;
		}
	}
}
#endif /* defined(CONFIG_SYS_POWER_MANAGEMENT) */
