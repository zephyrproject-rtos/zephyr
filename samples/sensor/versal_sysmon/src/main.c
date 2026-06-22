/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#include "versal_sysmon.h"

#define SYSMON_NODE DT_NODELABEL(sysmon0)

#if !DT_NODE_HAS_STATUS_OKAY(SYSMON_NODE)
#error "SYSMON node is not okay in devicetree (DT_NODELABEL(sysmon0))"
#endif

static const struct device *const sysmon_dev = DEVICE_DT_GET(SYSMON_NODE);

static int print_die_temperature(const struct device *dev)
{
	struct sensor_value val;
	int rc;

	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return rc;
	}
	rc = sensor_channel_get(dev, (enum sensor_channel)VERSAL_SYSMON_DIE_TEMP, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return rc;
	}
	printk("Sysmon Die temperature: %d °mC\n", val.val1);
	return 0;
}

int main(void)
{
	if (!device_is_ready(sysmon_dev)) {
		printk("SYSMON device not ready\n");
		return 0;
	}

	printk("SYSMON sample started\n");
	while (1) {
		int rc = print_die_temperature(sysmon_dev);

		if (rc < 0) {
			return 0;
		}

		k_msleep(1000);
	}
	return 0;
}
