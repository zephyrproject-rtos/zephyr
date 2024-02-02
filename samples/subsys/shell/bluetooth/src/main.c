/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern int bt_app_init(void);

int main(void)
{
	int err = -1;

	err = bt_app_init();
	if (err) {
		printk("Bluetooth failed to initialize: %d\n", err);
		return err;
	}

	printk("Shell over Bluetooth Sample started!\n");

	return 0;
}
