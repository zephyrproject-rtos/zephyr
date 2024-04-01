/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(freq_meter));
	struct sensor_value freq, rpm;

	if (dev == NULL || !device_is_ready(dev)) {
		printf("Could not get freq_meter device\n");
		return 0;
	}

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_FREQUENCY, &freq);
		sensor_channel_get(dev, SENSOR_CHAN_RPM, &rpm);

		printf("Freq=%dHz\tRPM=%d\n", freq.val1, rpm.val1);

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
