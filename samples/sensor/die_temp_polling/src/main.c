/*
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define DIE_TEMP_ALIAS(i) DT_ALIAS(_CONCAT(die_temp, i))
#define DIE_TEMPERATURE_SENSOR(i, _)                                                               \
	IF_ENABLED(DT_NODE_EXISTS(DIE_TEMP_ALIAS(i)), (DEVICE_DT_GET(DIE_TEMP_ALIAS(i)),))

/* support up to 16 cpu die temperature sensors */
static const struct device *const sensors[] = {LISTIFY(16, DIE_TEMPERATURE_SENSOR, ())};

static int print_die_temperature(const struct device *dev)
{
	struct sensor_value val;
	int rc;

	/* fetch sensor samples */
	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return rc;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return rc;
	}

	printk("CPU Die temperature[%s]: %.1f °C\n", dev->name, sensor_value_to_double(&val));
	return 0;
}

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
			rc = print_die_temperature(sensors[i]);
			if (rc < 0) {
				return 0;
			}
		}
		k_msleep(300);
	}
	return 0;
}
