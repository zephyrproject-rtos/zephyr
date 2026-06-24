/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   10

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct device *const qdc_dev = DEVICE_DT_GET(DT_ALIAS(eqdc0));

int main(void)
{
    struct sensor_value rotation;
    int rc;

    if (!device_is_ready(qdc_dev)) {
        printk("Error: The QDC0 device is not ready.\n");
        return 0;
    }

	while (1) {
		rc = sensor_sample_fetch(qdc_dev);
		if (rc != 0) {
			printk("Error reading QDC0 (%d)\n", rc);
			k_msleep(1000);
			continue;
		}

		rc = sensor_channel_get(qdc_dev, SENSOR_CHAN_ROTATION, &rotation);
		if (rc == 0) {
            /*Printing motor position */
			printk("Motor Position : %d\n", rotation.val1);
		} else {
			printk("Error reading channel (%d)\n", rc);
		}

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
