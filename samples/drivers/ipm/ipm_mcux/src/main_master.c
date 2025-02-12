/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>

int gcounter;

void ping_ipm_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	gcounter = *(int *)data;
	/* Show current ping-pong counter value */
	printk("Received: %d\n", gcounter);
	/* Increment on our side */
	gcounter++;
	if (gcounter < 100) {
		/* Send back to the other core */
		ipm_send(dev, 1, 0, &gcounter, 4);
	}
}

int main(void)
{
	int first_message = 1; /* do not start from 0,
				* zero value can't be sent via mailbox register
				*/
	const struct device *ipm;

	printk("Hello World from MASTER! %s\n", CONFIG_ARCH);

	/* Get IPM device handle */
	ipm = DEVICE_DT_GET_ANY(nxp_lpc_mailbox);
	if (!(ipm && device_is_ready(ipm))) {
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
	return 0;
}
