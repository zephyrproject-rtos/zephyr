/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>


/*
 * @file
 * @brief Hello World demo
 */


void main(void)
{
	printk("Hello World!\n");
}

#if DT_NODE_EXISTS(DT_INST(0, vnd_gpio))
/* Fake GPIO device, needed for building drivers that use DEVICE_DT_GET()
 * to access GPIO controllers.
 */
DEVICE_DT_DEFINE(DT_INST(0, vnd_gpio), NULL, NULL, NULL, NULL,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
#endif
