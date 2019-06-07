#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>
#include <misc/util.h>

void main(void)
{
    struct device *dev = device_get_binding("icm20948");

	if (dev == NULL) {
		printk("Could not get ICM20948 device\n");
		return;
	}

	printf("dev %p name %s\n", dev, dev->config->name);

	while (1) {
		struct sensor_value acc[3];

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &acc);

		printk("x: %d.%06d; y: %d.%06d; z: %d.%06d\n",
		      acc[0].val1, acc[0].val2, acc[1].val1, acc[1].val2,
		      acc[2].val1, acc[2].val2);

		k_sleep(500);

	}
}