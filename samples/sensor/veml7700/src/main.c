/*
 * Copyright (c) 2024 GOLLA MANOJ KUMAR
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

static struct sensor_value fid;

static void fetching_device_details(const struct device *dev)
{
	int ret;
	static int count=0;

	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printk("Sample Fetch Error %d\n", ret);
		return;
	}

	if(sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &fid) < 0) {
		printk("Channel Get Error %d\n", ret);
		return;
	}
	++count;
	printk("Ambient Light Sensor Data: %.1f\n", sensor_value_to_double(&fid));
	printk("count:%d\n",count);
}

int main(void)
{

	const struct device *i2c_dev = DEVICE_DT_GET_ONE(vishay_veml7700);
	if (!i2c_dev) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	while(1) {
		fetching_device_details(i2c_dev);
		k_msleep(1000);
		}
	return 0;
}
