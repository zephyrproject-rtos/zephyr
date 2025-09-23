/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usbd.h>

#include <sample_usbd.h>

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		printk("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		printk("Failed to enable USB");
		return 0;
	}

	printk("Bluetooth over USB sample\n");
	return 0;
}
