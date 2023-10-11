/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hello_world_driver.h"
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/syscall_handler.h>

/**
 * This is a minimal example of an out-of-tree driver
 * implementation. See the header file of the same name for details.
 */

static struct hello_world_dev_data {
	uint32_t foo;
} data;

static int init(const struct device *dev)
{
	data.foo = 5;

	return 0;
}

static void print_impl(const struct device *dev)
{
	printk("Hello World from the kernel: %d\n", data.foo);

	__ASSERT(data.foo == 5, "Device was not initialized!");
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_hello_world_print(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_HELLO_WORLD(dev, print));

	z_impl_hello_world_print(dev);
}
#include <syscalls/hello_world_print_mrsh.c>
#endif /* CONFIG_USERSPACE */


DEVICE_DEFINE(hello_world, "CUSTOM_DRIVER",
		    init, NULL, &data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &((struct hello_world_driver_api){ .print = print_impl }));
