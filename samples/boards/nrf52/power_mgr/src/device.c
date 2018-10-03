/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

static struct device *gpiob;
static struct device *device_list;
static int device_count;

/*
 * Example ordered list to store devices on which
 * device power policies would be executed.
 */
#define DEVICE_POLICY_MAX 15
static char device_ordered_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];

/* Initialize and configure GPIO */
void gpio_setup(void)
{

	gpiob = device_get_binding(PORT);

	/* Configure button 1 as deep sleep trigger event */
	gpio_pin_configure(gpiob, BUTTON_1, GPIO_DIR_IN
				| GPIO_PUD_PULL_UP);

	/* Configure button 2 as wake source from deep sleep */
	gpio_pin_configure(gpiob, BUTTON_2, GPIO_DIR_IN
				| GPIO_PUD_PULL_UP
				| GPIO_INT | GPIO_INT_LEVEL
				| GPIO_CFG_SENSE_LOW);

	gpio_pin_enable_callback(gpiob, BUTTON_2);
}

void suspend_devices(void)
{

	for (int i = device_count - 1; i >= 0; i--) {
		int idx = device_ordered_list[i];

		/* If necessary  the policy manager can check if a specific
		 * device in the device list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		device_retval[i] = device_set_power_state(&device_list[idx],
						DEVICE_PM_SUSPEND_STATE);
	}
}

void resume_devices(void)
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

void create_device_list(void)
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
	 */
	device_list_get(&device_list, &count);
	device_count = 4; /* Reserve for 32KHz, 16MHz, system clock, and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "clk_k32src")) {
			device_ordered_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "clk_m16src")) {
			device_ordered_list[1] = i;
		} else if (!strcmp(device_list[i].config->name, "sys_clock")) {
			device_ordered_list[2] = i;
		} else if (!strcmp(device_list[i].config->name, "UART_0")) {
			device_ordered_list[3] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
}
