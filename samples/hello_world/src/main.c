/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <dfu/mcuboot.h>

void main(void)
{
	int ret = -1;

	printk("Hello World! 2 %s\n", CONFIG_BOARD);

	/* The image of application needed be confirmed */
	printk("Confirming the boot image\n");
	ret = boot_write_img_confirmed();
	if (ret < 0) {
		printk("Error to confirm the image\n");
	} else {
		printk("Image is OK\n");
	}
}
