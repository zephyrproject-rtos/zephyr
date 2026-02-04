/*
 * Copyright The Zephyr Project Contributors
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

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
	return buf;
}

static int process_icm20948(const struct device *dev)
{
	struct sensor_value temperature;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	struct sensor_value magn[3];
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
	}
	if (rc == 0) {
		printf("[%s]: %g Cel\n"
		       "  accel %f %f %f m/s/s\n"
		       "  gyro  %f %f %f rad/s\n",
		       now_str(), sensor_value_to_double(&temperature),
		       sensor_value_to_double(&accel[0]), sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]), sensor_value_to_double(&gyro[0]),
		       sensor_value_to_double(&gyro[1]), sensor_value_to_double(&gyro[2]));
	}

	/* Magnetometer may return -ENOTSUP if disabled */
	int magn_rc = sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, magn);

	if (magn_rc == 0) {
		printf("  magn  %f %f %f uT\n", sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]), sensor_value_to_double(&magn[2]));
	} else if (magn_rc == -EOVERFLOW) {
		printf("  magn  <overflow>\n");
	} else {
		printf("  magn  <error: %d>\n", magn_rc);
	}

	if (rc != 0) {
		printf("sample fetch/get failed: %d\n", rc);
	}

	return rc;
}

#ifdef CONFIG_ICM20948_TRIGGER
static struct sensor_trigger trigger;

static void handle_icm20948_drdy(const struct device *dev, const struct sensor_trigger *trig)
{
	int rc = process_icm20948(dev);

	if (rc != 0) {
		printf("cancelling trigger due to failure: %d\n", rc);
		(void)sensor_trigger_set(dev, trig, NULL);
		return;
	}
}
#endif /* CONFIG_ICM20948_TRIGGER */

int main(void)
{
	const struct device *const icm20948 = DEVICE_DT_GET_ONE(invensense_icm20948);

	if (!device_is_ready(icm20948)) {
		printf("Device %s is not ready\n", icm20948->name);
		return 0;
	}

	printf("Found ICM-20948 device %s\n", icm20948->name);

#ifdef CONFIG_ICM20948_TRIGGER
	trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	if (sensor_trigger_set(icm20948, &trigger, handle_icm20948_drdy) < 0) {
		printf("Cannot configure trigger\n");
		return 0;
	}
	printf("Configured for triggered sampling.\n");
#endif

	while (!IS_ENABLED(CONFIG_ICM20948_TRIGGER)) {
		int rc = process_icm20948(icm20948);

		if (rc != 0) {
			break;
		}
		k_sleep(K_SECONDS(2));
	}

	/* triggered runs with its own thread after exit */
	return 0;
}
