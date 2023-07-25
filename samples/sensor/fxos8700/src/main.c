/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */

#if !defined(CONFIG_FXOS8700_TRIGGER_NONE)
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);

	/* Always fetch the sample to clear the data ready interrupt in the
	 * sensor.
	 */
	if (sensor_sample_fetch(dev)) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	k_sem_give(&sem);
}
#endif /* !CONFIG_FXOS8700_TRIGGER_NONE */

int main(void)
{
	struct sensor_value accel[3];
	const struct device *const dev = DEVICE_DT_GET_ONE(nxp_fxos8700);

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	struct sensor_value attr = {
		.val1 = 6,
		.val2 = 250000,
	};

	if (sensor_attr_set(dev, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr)) {
		printk("Could not set sampling frequency\n");
		return 0;
	}

#ifdef CONFIG_FXOS8700_MOTION
	attr.val1 = 10;
	attr.val2 = 600000;
	if (sensor_attr_set(dev, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SLOPE_TH, &attr)) {
		printk("Could not set slope threshold\n");
		return 0;
	}
#endif

#if !defined(CONFIG_FXOS8700_TRIGGER_NONE)
	struct sensor_trigger trig = {
#ifdef CONFIG_FXOS8700_MOTION
		.type = SENSOR_TRIG_DELTA,
#else
		.type = SENSOR_TRIG_DATA_READY,
#endif
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return 0;
	}
#endif /* !CONFIG_FXOS8700_TRIGGER_NONE */

	while (1) {
#ifdef CONFIG_FXOS8700_TRIGGER_NONE
		k_sleep(K_MSEC(160));
		sensor_sample_fetch(dev);
#else
		k_sem_take(&sem, K_FOREVER);
#endif
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
		/* Print accel x,y,z data */
		printf("AX=%10.6f AY=%10.6f AZ=%10.6f ",
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
#if defined(CONFIG_FXOS8700_MODE_MAGN) || defined(CONFIG_FXOS8700_MODE_HYBRID)
		struct sensor_value magn[3];

		sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, magn);
		/* Print mag x,y,z data */
		printf("MX=%10.6f MY=%10.6f MZ=%10.6f ",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));
#endif
#ifdef CONFIG_FXOS8700_TEMP
		struct sensor_value temp;

		sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp);
		/* Print accel x,y,z and mag x,y,z data */
		printf("T=%10.6f", sensor_value_to_double(&temp));
#endif
		printf("\n");
	}
}
