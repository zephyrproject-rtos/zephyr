/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <drivers/sensor.h>

LOG_MODULE_REGISTER(app);

static const struct device *get_bmp180_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(bosch_bmp180);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		LOG_ERR("Error: no device found.");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Error: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.",
		       dev->name);
		return NULL;
	}

	LOG_INF("Found device \"%s\", getting sensor data", dev->name);
	return dev;
}

void main(void) {

	k_sleep(K_SECONDS(5));
	LOG_INF("started!");

	const struct device *dev_bmp_180 = get_bmp180_device();

	if (dev_bmp_180 == NULL) {
		LOG_ERR("bmp180 not found");
		return;
	}

	while (1) {
		struct sensor_value temp, press, altitude;

		sensor_sample_fetch(dev_bmp_180);
		sensor_channel_get(dev_bmp_180, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev_bmp_180, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dev_bmp_180, SENSOR_CHAN_ALTITUDE, &altitude);

		printk("\r\ntemperature: %d.%d C\r\npressure: %d Pa\r\nAltitude: %d m above sea level\r\n",
			temp.val1, temp.val2/100000, press.val1, altitude.val1);

		k_sleep(K_MSEC(5000));
	}
}
