/*
 * Copyright (c) 2019, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>


static int32_t read_sensor(const struct device *sensor)
{
	struct sensor_value val[3];
	int32_t ret = 0;

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
	const struct device *dev;

	dev = device_get_binding(DT_LABEL(DT_INST(0, honeywell_hmc5883l)));

	if (dev == NULL) {
		printk("Could not get %s device at I2C addr 0x%02X\n",
		       DT_LABEL(DT_INST(0, honeywell_hmc5883l)),
		       DT_REG_ADDR(DT_INST(0, honeywell_hmc5883l)));
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	while (1) {
		read_sensor(dev);
		k_sleep(K_MSEC(1000));
	}
}
