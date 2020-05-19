/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hello_world_driver.h"
#include <stdio.h>
#include <zephyr.h>

static void user_entry(void *p1, void *p2, void *p3)
{
	struct device *dev = p1;

	hello_world_print(dev);
}

void main(void)
{
	printk("Hello World from the app!\n");

	struct device *dev = device_get_binding("CUSTOM_DRIVER");

	__ASSERT(dev, "Failed to get device binding");

	printk("device is %p, name is %s\n", dev, dev->name);

	k_object_access_grant(dev, k_current_get());
	k_thread_user_mode_enter(user_entry, dev, NULL, NULL);
}
