/*
 * Copyright (c) 2023 Kenneth J. Miller <ken@miller.ec>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define VOLT_SENSOR_ALIAS(i) DT_ALIAS(_CONCAT(volt_sensor, i))
#define VOLT_SENSOR(i, _)                                                                          \
	IF_ENABLED(DT_NODE_EXISTS(VOLT_SENSOR_ALIAS(i)), (DEVICE_DT_GET(VOLT_SENSOR_ALIAS(i)),))

/* support up to 16 voltage sensors */
static const struct device *const sensors[] = {LISTIFY(16, VOLT_SENSOR, ())};

static int print_voltage(const struct device *dev)
{
	struct sensor_value val;
	int rc;

	/* fetch sensor samples */
	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return 0;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return 0;
	}

	printk("Sensor voltage[%s]: %.2f V\n", dev->name, sensor_value_to_double(&val));
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

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		rc = print_voltage(sensors[i]);
		if (rc < 0) {
			return 0;
		}
	}

	return 0;
}
