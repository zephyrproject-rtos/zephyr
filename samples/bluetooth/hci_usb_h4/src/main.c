/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>

int main(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Failed to enable USB");
		return 0;
	}

	printk("Bluetooth H:4 over USB sample\n");
	return 0;
}
