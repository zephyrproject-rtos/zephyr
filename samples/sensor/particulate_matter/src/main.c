/*
 * Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#define PARTICLE_ALIAS(i) DT_ALIAS(_CONCAT(particle, i))
#define PARTICLE_SENSOR(i, _)                                                               \
	IF_ENABLED(DT_NODE_EXISTS(PARTICLE_ALIAS(i)), (DEVICE_DT_GET(PARTICLE_ALIAS(i)),))

static const struct device *const sensors[] = {LISTIFY(10, PARTICLE_SENSOR, ())};

static int print_particle_values(const struct device *dev)
{
	int ret;
	struct sensor_value sensor_values_particle_count[6];
	struct sensor_value sensor_values_pm_1_0;
	struct sensor_value sensor_values_pm_2_5;
	struct sensor_value sensor_values_pm_10;

	ret = sensor_sample_fetch(dev);
	if (ret) {
		printk("Failed to fetch a sample, %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PARTICLE_COUNT,
					&sensor_values_particle_count[0]);
	if (ret) {
		printk("Failed to get particle count values, %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PM_1_0, &sensor_values_pm_1_0);
	if (ret) {
		printk("Failed to get pm_1_0 value, %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PM_2_5, &sensor_values_pm_2_5);
	if (ret) {
		printk("Failed to get pm_1_0 value, %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PM_10, &sensor_values_pm_10);
	if (ret) {
		printk("Failed to get pm_1_0 value, %d\n", ret);
		return ret;
	}

	printk("new sample:\n");
	printk("pc0_5: %i\n", sensor_values_particle_count[0].val1);
	printk("pc1_0: %i\n", sensor_values_particle_count[1].val1);
	printk("pc2_5: %i\n", sensor_values_particle_count[2].val1);
	printk("pc5_0: %i\n", sensor_values_particle_count[3].val1);
	printk("pc7_5: %i\n", sensor_values_particle_count[4].val1);
	printk("pc10_0: %i\n", sensor_values_particle_count[5].val1);
	printk("pm1_0: %i.%i\n", sensor_values_pm_1_0.val1, sensor_values_pm_1_0.val2);
	printk("pm2_5: %i.%i\n", sensor_values_pm_2_5.val1, sensor_values_pm_2_5.val2);
	printk("pm10_0: %i.%i\n\n", sensor_values_pm_10.val1, sensor_values_pm_10.val2);

	return 0;
}

int main(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
		printk("Found device \"%s\", getting sensor data\n", sensors[i]->name);
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			ret = print_particle_values(sensors[i]);
			if (ret < 0) {
				return 0;
			}
		}
		k_msleep(1000);
	}
	return 0;
}
