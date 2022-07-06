/*
 * Copyright (c) 2020 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(maxim_max17262);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	while (1) {
		struct sensor_value voltage, avg_current, temperature;
		float i_avg;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT,
						  &avg_current);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &temperature);

		i_avg = avg_current.val1 + (avg_current.val2 / 1000000.0);

		printk("V: %d.%06d V; I: %f mA; T: %d.%06d °C\n",
		      voltage.val1, voltage.val2, (double)i_avg,
		      temperature.val1, temperature.val2);

		k_sleep(K_MSEC(1000));
	}
}
