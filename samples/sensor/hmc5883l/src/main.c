/*
 * Copyright (c) 2019, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/printk.h>


static s32_t read_sensor(struct device *sensor)
{
	struct sensor_value val[3];
	s32_t ret = 0;

	ret = sensor_sample_fetch(sensor);
	if (ret) {
		printk("sensor_sample_fetch failed ret %d\n", ret);
		goto end;
	}

	ret = sensor_channel_get(sensor, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		printf("Cannot read sensor channels\n");
		goto end;
	}

	printf("( x y z ) = ( %f  %f  %f )\n", sensor_value_to_double(&val[0]),
	       sensor_value_to_double(&val[1]),
	       sensor_value_to_double(&val[2]));

end:
	return ret;
}

void main(void)
{
	struct device *dev;

	dev = device_get_binding(DT_INST_0_HONEYWELL_HMC5883L_LABEL);

	if (dev == NULL) {
		printk("Could not get %s device at I2C addr 0x%02X\n",
		       DT_INST_0_HONEYWELL_HMC5883L_LABEL,
		       DT_INST_0_HONEYWELL_HMC5883L_BASE_ADDRESS);
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->config->name);

	while (1) {
		read_sensor(dev);
		k_sleep(K_MSEC(1000));
	}
}
