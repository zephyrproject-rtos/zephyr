/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hello_world_driver.h"
#include <zephyr/types.h>

/**
 * This is a minimal example of an out-of-tree driver
 * implementation. See the header file of the same name for details.
 */

static struct hello_world_dev_data {
	uint32_t foo;
} data;

static int init(struct device *dev)
{
	data.foo = 5;

	return 0;
}

static void print_impl(struct device *dev)
{
	printk("Hello World from the kernel: %d\n", data.foo);

	__ASSERT(data.foo == 5, "Device was not initialized!");
}

DEVICE_AND_API_INIT(hello_world, "CUSTOM_DRIVER",
		    init, &data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &((struct hello_world_driver_api){ .print = print_impl }));
