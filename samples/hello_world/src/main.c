/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>

extern const struct device *__pm_device_start[];
extern const struct device *__pm_device_end[];

extern const struct device __device_start[];
extern const struct device __device_end[];

void main(void)
{
	size_t pm_devices_cnt = __pm_device_end - __pm_device_start;

	printk("%d devices provide power management\n", pm_devices_cnt);
	for (size_t i = 0U; i < pm_devices_cnt; i++) {
		printk("- %s\n", __pm_device_start[i]->name);
	}

	size_t devices_cnt = __device_end - __device_start;

	printk("%d devices available\n", devices_cnt);
	for (size_t i = 0U; i < devices_cnt; i++) {
		printk("- %s\n", __device_start[i].name);
	}
}
