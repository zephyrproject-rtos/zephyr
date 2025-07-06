/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/vl53l0x.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_vl53l0x);
	struct sensor_value value;
	int ret;

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return 0;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &value);
		printk("prox is %d\n", value.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &value);
		printk("distance is %.3lld mm\n", sensor_value_to_milli(&value));

		ret = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_DMAX, &value);
		printk("Max distance is %.3lld mm\n", sensor_value_to_milli(&value));

		ret = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_SIGNAL_RATE_RTN_CPS, &value);
		printk("Signal rate is %d Cps\n", value.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_AMBIENT_RATE_RTN_CPS, &value);
		printk("Ambient rate is %d Cps\n", value.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_EFFECTIVE_SPAD_RTN_COUNT, &value);
		printk("SPADs used: %d\n", value.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_STATUS, &value);
		if (value.val1 == VL53L0X_RANGE_STATUS_RANGE_VALID) {
			printk("Status: OK\n");
		} else {
			printk("Status: Error code %d\n", value.val1);
		}

		printk("\n");
		k_sleep(K_MSEC(5000));
	}
	return 0;
}
