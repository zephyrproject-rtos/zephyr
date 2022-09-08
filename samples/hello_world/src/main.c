/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

void main(void)
{
	printk("Hello World! %p\n", DEVICE_DT_GET(DT_INVALID_NODE));
}
