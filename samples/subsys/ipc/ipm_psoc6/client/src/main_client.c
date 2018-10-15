/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cy_sysint.h"
#include <misc/printk.h>
#include <zephyr.h>
#include <ipm.h>

#define SLEEP_TIME 2000

#define COUNTER_MAX_VAL 10

void message_ipm_callback(void *context, u32_t id, volatile void *data)
{
	struct device *ipm = (struct device *)context;
	u32_t *data32 = (u32_t *)data;
	int ipm_error;

	printk("Received counter value: %u\n", *data32);
	*data32 = *data32 + 1;

	if (*data32 < COUNTER_MAX_VAL) {
		printk("Sending next message: %u\n", *data32);
		ipm_error = ipm_send(ipm, 1, 0, data32, 4);

		if (ipm_error != 0) {
			printk("ipm_send returned error %i\n", ipm_error);
		}
	} else {
		printk("The last message cycle finished\n");
	}
}

void main(void)
{
	struct device *ipm;
	int ipm_error;
	u32_t counter = 0;

	ipm = device_get_binding(DT_CYPRESS_PSOC6_MAILBOX_PSOC6_IPM0_LABEL);
	ipm_register_callback(ipm, message_ipm_callback, ipm);

	printk("PSoC6 IPM Client example has started\n");
	printk("Sending first message \" %u \" from %u messages via IPM\n",
		counter, COUNTER_MAX_VAL);
	ipm_error = ipm_send(ipm, 1, 0, &counter, 4);
	if (ipm_error != 0) {
		printk("ipm_send returned error %i\n", ipm_error);
	}

	while (1) {
		k_sleep(SLEEP_TIME);
	}
}

