/*
 * Copyright (c) 2025 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/sensor.h>

#include <stdio.h>

static void stts22h_config(const struct device *stts22h)
{
	struct sensor_value odr_attr;

	/* Set STTS22H ODR to 100 Hz (or as close as supported) */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;
	if (sensor_attr_set(stts22h, SENSOR_CHAN_AMBIENT_TEMP,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for STTS22H\n");
		return;
	}
}

int main(void)
{
	const struct device *const stts22h = DEVICE_DT_GET_ONE(st_stts22h);
	int ret;

	printf("Zephyr STTS22H sensor sample. Board: %s\n", CONFIG_BOARD);

	if (!device_is_ready(stts22h)) {
		printk("%s: device not ready.\n", stts22h->name);
		return 0;
	}

	stts22h_config(stts22h);

	while (1) {
		struct sensor_value temp;

		ret = sensor_sample_fetch(stts22h);
		if (ret < 0) {
			printk("STTS22H sample fetch error (%d)\n", ret);
			k_msleep(1000);
			continue;
		}

		ret = sensor_channel_get(stts22h, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		if (ret < 0) {
			printk("STTS22H channel read error (%d)\n", ret);
			k_msleep(1000);
			continue;
		}

		printf("[%6lld ms] Temperature: %.1f C\n", k_uptime_get(),
			sensor_value_to_double(&temp));

		k_msleep(2000);
	}
}
