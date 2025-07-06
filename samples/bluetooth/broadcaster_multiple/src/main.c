/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>

int broadcaster_multiple(void);

int main(void)
{
	int err;

	printk("Starting Multiple Broadcaster Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	(void)broadcaster_multiple();

	printk("Exiting %s thread.\n", __func__);
	return 0;
}
