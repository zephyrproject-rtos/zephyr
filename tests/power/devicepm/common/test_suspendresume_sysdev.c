/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <string.h>
#include <device.h>


#define DEVICE_POLICY_MAX 15
static struct device *device_list;
static int device_count;
static char device_ordered_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];
static int init;

static void populate_device_list(void)
{
	int count;
	int i;

	/*
	 * Following is an example of how the device list can be used
	 * to suspend devices based on custom policies.
	 *
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 *
	 * Other devices depend on APICs so ioapic and loapic devices
	 * will be placed first in the device ordered list. Move any
	 * other devices to the beginning as necessary. e.g. uart
	 * is useful to enable early prints.
	 */
	device_list_get(&device_list, &count);

#if (CONFIG_X86)
	device_count = 3; /* Reserve for ioapic, loapic and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "loapic")) {
			device_ordered_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "ioapic")) {
			device_ordered_list[1] = i;
		} else if (!strcmp(device_list[i].config->name,
					CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
			device_ordered_list[2] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
#elif (CONFIG_ARC)
	device_count = 1; /* Reserve for irq unit */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "arc_v2_irq_unit")) {
			device_ordered_list[0] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
#endif
}

void suspend_sysdev(int state)
{
	int i;

	for (i = device_count - 1; i >= 0; i--) {
		int idx = device_ordered_list[i];

		device_retval[i] = device_set_power_state(&device_list[idx],
						DEVICE_PM_SUSPEND_STATE);
	}
}

void resume_sysdev(int state)
{
	int i;

	for (i = 0; i < device_count; i++) {
		if (!device_retval[i]) {
			int idx = device_ordered_list[i];

			device_set_power_state(&device_list[idx],
						DEVICE_PM_ACTIVE_STATE);
		}
	}
}

void test_sysdev_state(int state)
{
	if (!init) {
		populate_device_list();
		init = 1;
	}
	if (state == DEVICE_PM_ACTIVE_STATE) {
		resume_sysdev(state);
	} else {
		suspend_sysdev(state);
	}
}
