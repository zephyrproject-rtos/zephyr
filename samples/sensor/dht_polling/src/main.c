/*
 * Copyright (c) 2023 Ian Morris
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>

#define DHT_ALIAS(i) DT_ALIAS(_CONCAT(dht, i))
#define DHT_DEVICE(i, _)                                                                 \
	IF_ENABLED(DT_NODE_EXISTS(DHT_ALIAS(i)), (DEVICE_DT_GET(DHT_ALIAS(i)),))

/* Support up to 10 temperature/humidity sensors */
static const struct device *const sensors[] = {LISTIFY(10, DHT_DEVICE, ())};

int main(void)
{
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			struct device *dev = (struct device *)sensors[i];

			rc = sensor_sample_fetch(dev);
			if (rc < 0) {
				printk("%s: sensor_sample_fetch() failed: %d\n", dev->name, rc);
				return rc;
			}

			struct sensor_value temp;
			struct sensor_value hum;

			rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (rc == 0) {
				rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum);
			}
			if (rc != 0) {
				printf("get failed: %d\n", rc);
				break;
			}

			printk("%16s: temp is %d.%02d °C humidity is %d.%02d %%RH\n",
							dev->name, temp.val1, temp.val2 / 10000,
							hum.val1, hum.val2 / 10000);
		}
		k_msleep(1000);
	}
	return 0;
}
