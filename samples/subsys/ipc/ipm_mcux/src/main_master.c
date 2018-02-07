/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <ipm.h>

struct device *ipm;
int gcounter;

void ping_ipm_callback(void *context, u32_t id, volatile void *data)
{
	gcounter = *(int *)data;
	/* Show current ping-pong counter value */
	printk("Received: %d\n", gcounter);
	/* Increment on our side */
	gcounter++;
	if (gcounter < 100) {
		/* Send back to the other core */
		ipm_send(ipm, 1, 0, &gcounter, 4);
	}
}

void main(void)
{
	int first_message = 1; /* do not start from 0,
				* zero value can't be sent via mailbox register
				*/
	printk("Hello World from MASTER! %s\n", CONFIG_ARCH);

	/* Get IPM device handle */
	ipm = device_get_binding(MAILBOX_0_LABEL);
	if (!ipm) {
		printk("Could not get IPM device handle!\n");
		while (1) {
		}
	}

	/* Register application callback with no context */
	ipm_register_callback(ipm, ping_ipm_callback, NULL);
	/* Enable the IPM device */
	ipm_set_enabled(ipm, 1);

	/* Send initial message with 4 bytes length*/
	ipm_send(ipm, 1, 0, &first_message, 4);
	while (1) {
	}
}
