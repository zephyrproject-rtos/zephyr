/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hello_world_driver.h"
#include <stdio.h>
#include <zephyr.h>

void main(void)
{
	printk("Hello World from the app!\n");

	struct device *dev = device_get_binding("CUSTOM_DRIVER");

	__ASSERT(dev, "Failed to get device binding");

	printk("device is %p, name is %s\n", dev, dev->name);

	hello_world_print(dev);
}
