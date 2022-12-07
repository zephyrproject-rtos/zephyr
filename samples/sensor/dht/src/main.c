/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

void main(void)
{
	const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);

	if (!device_is_ready(dht22)) {
		printf("Device %s is not ready\n", dht22->name);
		return;
	}

	while (true) {
		int rc = sensor_sample_fetch(dht22);

		if (rc != 0) {
			printf("Sensor fetch failed: %d\n", rc);
			break;
		}

		struct sensor_value temperature;
		struct sensor_value humidity;

		rc = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP,
					&temperature);
		if (rc == 0) {
			rc = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY,
						&humidity);
		}
		if (rc != 0) {
			printf("get failed: %d\n", rc);
			break;
		}

		printf("[%s]: %.1f Cel ; %.1f %%RH\n",
		       now_str(),
		       sensor_value_to_double(&temperature),
		       sensor_value_to_double(&humidity));
		k_sleep(K_SECONDS(2));
	}
}
