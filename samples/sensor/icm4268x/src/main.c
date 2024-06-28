/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

K_SEM_DEFINE(sem_icm42688, 0, 1);	/* starts off "not available" */
K_SEM_DEFINE(sem_icm42686, 0, 1);	/* starts off "not available" */

static void trigger_handler_icm42688(const struct device *dev, const struct sensor_trigger *trigger)
{
	k_sem_give(&sem_icm42688);
}

static void trigger_handler_icm42686(const struct device *dev, const struct sensor_trigger *trigger)
{
	k_sem_give(&sem_icm42686);
}

int main(void)
{
	const struct device *const icm42688 = DEVICE_DT_GET_ONE(invensense_icm42688);
	const struct device *const icm42686 = DEVICE_DT_GET_ONE(invensense_icm42686);
	struct sensor_value gyro[3], accel[3];

	if (!device_is_ready(icm42688) || !device_is_ready(icm42686)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_GYRO_XYZ,
	};

	if (sensor_trigger_set(icm42688, &trig, trigger_handler_icm42688) ||
	    sensor_trigger_set(icm42686, &trig, trigger_handler_icm42686)) {
		printf("Could not set trigger\n");
		return 0;
	}

	while (1) {
		k_sem_take(&sem_icm42688, K_FOREVER);
		sensor_sample_fetch(icm42688);
		sensor_channel_get(icm42688, SENSOR_CHAN_GYRO_XYZ, gyro);
		sensor_channel_get(icm42688, SENSOR_CHAN_ACCEL_XYZ, accel);

		printf("icm42688 gyro X = val1 %d val2 %d; Y = val1 %d val2 %d; Z = val1 %d val2 %d;\n",
		       (gyro[0].val1), (gyro[0].val2),
		       (gyro[1].val1), (gyro[1].val2),
		       (gyro[2].val1), (gyro[2].val2));
		printf("icm42688 accel X = val1 %d val2 %d; Y = val1 %d val2 %d; Z = val1 %d val2 %d;\n",
		       (accel[0].val1), (accel[0].val2),
		       (accel[1].val1), (accel[1].val2),
		       (accel[2].val1), (accel[2].val2));

		k_sleep(K_MSEC(250));

		k_sem_take(&sem_icm42686, K_FOREVER);
		sensor_sample_fetch(icm42686);
		sensor_channel_get(icm42686, SENSOR_CHAN_GYRO_XYZ, gyro);
		sensor_channel_get(icm42686, SENSOR_CHAN_ACCEL_XYZ, accel);

		printf("icm42686 gyro X = val1 %d val2 %d; Y = val1 %d val2 %d; Z = val1 %d val2 %d;\n",
		       (gyro[0].val1), (gyro[0].val2),
		       (gyro[1].val1), (gyro[1].val2),
		       (gyro[2].val1), (gyro[2].val2));
		printf("icm42686 accel X = val1 %d val2 %d; Y = val1 %d val2 %d; Z = val1 %d val2 %d;\n",
		       (accel[0].val1), (accel[0].val2),
		       (accel[1].val1), (accel[1].val2),
		       (accel[2].val1), (accel[2].val2));

		k_sleep(K_MSEC(250));
	}

	return 0;
}
