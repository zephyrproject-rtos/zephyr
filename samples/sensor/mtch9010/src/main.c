/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor/mtch9010.h>
#include <zephyr/kernel.h>

int main(void)
{
	const struct device *my_sensor = DEVICE_DT_GET(
		DT_COMPAT_GET_ANY_STATUS_OKAY(microchip_mtch9010));

	if (!device_is_ready(my_sensor)) {
		printk("MTCH9010 is NOT Ready\n");
		return 0;
	}

	struct sensor_value val;

	while (1) {
		sensor_sample_fetch(my_sensor);

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_SW_OUT_STATE, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_SW_OUT_STATE\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_SW_OUT_STATE = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_OUT_STATE, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_OUT_STATE\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_OUT_STATE = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_REFERENCE_VALUE, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_REFERENCE_VALUE\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_REFERENCE_VALUE = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_MEAS_RESULT, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_MEAS_RESULT\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_MEAS_RESULT = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_MEAS_DELTA, &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_MEAS_DELTA\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_MEAS_DELTA = %d\r\n", val.val1);
		}

		if (sensor_channel_get(my_sensor, SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE,
				       &val) < 0) {
			printk("Unable to fetch SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE\r\n");
		} else {
			printk("SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE = %d\r\n", val.val1);
		}

		printk("\n");
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
