/*
 * Copyright (c) 2024 Nxp Semiconductor
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */

#if !defined(CONFIG_FXLS8974_TRIGGER_NONE)
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
#endif /* !CONFIG_FXLS8974_TRIGGER_NONE */

int main(void)
{
	struct sensor_value data[4];
	const struct device *const dev = DEVICE_DT_GET_ONE(nxp_fxls8974);

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

#if !defined(CONFIG_FXLS8974_TRIGGER_NONE)
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return 0;
	}
#endif /* !CONFIG_FXLS8974_TRIGGER_NONE */

	while (1) {
#ifdef CONFIG_FXLS8974_TRIGGER_NONE
		k_sleep(K_MSEC(160));
		sensor_sample_fetch(dev);
#else
		k_sem_take(&sem, K_FOREVER);
#endif
		sensor_channel_get(dev, SENSOR_CHAN_ALL, data);
		/* Print accel x,y,z data */
		printf("AX=%10.2f AY=%10.2f AZ=%10.2f TEMP=%dC",
		       sensor_value_to_double(&data[0]),
		       sensor_value_to_double(&data[1]),
		       sensor_value_to_double(&data[2]),
			data[3].val1);
		printf("\n");
	}
}
