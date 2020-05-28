/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HELLO_WORLD_DRIVER_H__
#define __HELLO_WORLD_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

/*
 * This 'Hello World' driver has a 'print' syscall that prints the
 * famous 'Hello World!' string.
 *
 * The string is formatted with some internal driver data to
 * demonstrate that drivers are initialized during the boot process.
 *
 * The driver exists to demonstrate (and test) custom drivers that are
 * maintained outside of Zephyr.
 */
__subsystem struct hello_world_driver_api {
	/* This struct has a member called 'print'. 'print' is function
	 * pointer to a function that takes 'struct device *dev' as an
	 * argument and returns 'void'.
	 */
	void (*print)(struct device *dev);
};

__syscall     void        hello_world_print(struct device *dev);
static inline void z_impl_hello_world_print(struct device *dev)
{
	const struct hello_world_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");

	api->print(dev);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/hello_world_driver.h>

#endif /* __HELLO_WORLD_DRIVER_H__ */
