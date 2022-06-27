/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>

void main(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Failed to enable USB");
		return;
	}

	printk("Bluetooth H:4 over USB sample\n");
}
