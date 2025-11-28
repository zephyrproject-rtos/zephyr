/*
 * Copyright (c) 2025 LCKFB
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <math.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qmi8658c, LOG_LEVEL_INF);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

/* Calculate tilt angles from accelerometer data
 * This function calculates the tilt angles (roll, pitch, yaw) from
 * accelerometer readings using atan2 function.
 */
static void calculate_angles(float acc_x, float acc_y, float acc_z,
			     float *angle_x, float *angle_y, float *angle_z)
{
	float temp;

	/* Calculate angle X (roll) */
	temp = acc_x / sqrtf(acc_y * acc_y + acc_z * acc_z);
	*angle_x = atanf(temp) * 57.3f;  /* Convert to degrees: 180/PI ≈ 57.3 */

	/* Calculate angle Y (pitch) */
	temp = acc_y / sqrtf(acc_x * acc_x + acc_z * acc_z);
	*angle_y = atanf(temp) * 57.3f;

	/* Calculate angle Z (yaw) */
	temp = acc_z / sqrtf(acc_x * acc_x + acc_y * acc_y);
	*angle_z = atanf(temp) * 57.3f;
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(qmi8658c_0));
	struct sensor_value accel[3], gyro[3];
	int ret;
	float acc_ms2[3], gyr_rads[3];
	float angle_x, angle_y, angle_z;

	if (!device_is_ready(dev)) {
		LOG_ERR("Sensor device not ready: %s", dev->name);
		return 0;
	}

	LOG_INF("QMI8658C sensor sample started");

	while (1) {
		/* Fetch sensor data */
		ret = sensor_sample_fetch(dev);
		if (ret) {
			if (ret != -EBUSY) {
				LOG_ERR("sensor_sample_fetch failed: %d", ret);
			}
			k_sleep(K_MSEC(10));
			continue;
		}

		/* Read accelerometer data (in m/s²) */
		ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
		if (ret) {
			LOG_ERR("sensor_channel_get(ACCEL) failed: %d", ret);
			k_sleep(K_MSEC(10));
			continue;
		}

		/* Read gyroscope data (in rad/s) */
		ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
		if (ret) {
			LOG_ERR("sensor_channel_get(GYRO) failed: %d", ret);
			k_sleep(K_MSEC(10));
			continue;
		}

		/* Convert sensor_value to float */
		acc_ms2[0] = sensor_value_to_float(&accel[0]);
		acc_ms2[1] = sensor_value_to_float(&accel[1]);
		acc_ms2[2] = sensor_value_to_float(&accel[2]);
		gyr_rads[0] = sensor_value_to_float(&gyro[0]);
		gyr_rads[1] = sensor_value_to_float(&gyro[1]);
		gyr_rads[2] = sensor_value_to_float(&gyro[2]);

		/* Calculate tilt angles from accelerometer */
		calculate_angles(acc_ms2[0], acc_ms2[1], acc_ms2[2],
				&angle_x, &angle_y, &angle_z);

		/* Display sensor data */
		printf("Accel: X=%.2f, Y=%.2f, Z=%.2f m/s² | "
		       "Gyro: X=%.2f, Y=%.2f, Z=%.2f rad/s | "
		       "Angle: X=%.1f°, Y=%.1f°, Z=%.1f°\n",
		       (double)acc_ms2[0], (double)acc_ms2[1], (double)acc_ms2[2],
		       (double)gyr_rads[0], (double)gyr_rads[1], (double)gyr_rads[2],
		       (double)angle_x, (double)angle_y, (double)angle_z);

		/* Sample at 10 Hz (100ms interval) */
		k_sleep(K_MSEC(100));
	}

	return 0;
}
