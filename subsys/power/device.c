/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <string.h>
#include <soc.h>
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
	DT_ETH_MCUX_0_NAME,
};
#else
#error "Add SoC's core devices list for PM"
#endif

/*
 * Ordered list to store devices on which
 * device power policies would be executed.
 */
static int device_ordered_list[MAX_PM_DEVICES];
static int device_retval[MAX_PM_DEVICES];
static struct device *pm_device_list;
static int device_count;

const char *device_pm_state_str(u32_t state)
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

static int _sys_pm_devices(u32_t state)
{
	for (int i = device_count - 1; i >= 0; i--) {
		int idx = device_ordered_list[i];

		/* TODO: Improve the logic by checking device status
		 * and set the device states accordingly.
		 */
		device_retval[i] = device_set_power_state(&pm_device_list[idx],
						state,
						NULL, NULL);
		if (device_retval[i]) {
			LOG_DBG("%s did not enter %s state",
				pm_device_list[idx].config->name,
				device_pm_state_str(state));
			return device_retval[i];
		}
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
	int i;

	for (i = 0; i < device_count; i++) {
		if (!device_retval[i]) {
			int idx = device_ordered_list[i];

			device_set_power_state(&pm_device_list[idx],
					DEVICE_PM_ACTIVE_STATE, NULL, NULL);
		}
	}
}

void sys_pm_create_device_list(void)
{
	int count;
	int i, j;
	bool is_core_dev;

	/*
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 */
	device_list_get(&pm_device_list, &count);

	/* Reserve for 32KHz, 16MHz, system clock, etc... */
	device_count = ARRAY_SIZE(core_devices);

	for (i = 0; (i < count) && (device_count < MAX_PM_DEVICES); i++) {

		/* Check if the device is core device */
		for (j = 0, is_core_dev = false;
		     j < ARRAY_SIZE(core_devices);
		     j++) {
			if (!strcmp(pm_device_list[i].config->name,
						core_devices[j])) {
				is_core_dev = true;
				break;
			}
		}

		if (is_core_dev) {
			device_ordered_list[j] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
}
#endif /* defined(CONFIG_SYS_POWER_MANAGEMENT) */
