/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bno055". (If there are multiple, just pick one.)
 */
static const struct device *get_bno055_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(bosch_bno055);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;


}

void main(void)
{
	const struct device *dev = get_bno055_device();

	if (dev == NULL) {
		return;
	}

	while (1) {
		struct sensor_value euler_h, euler_r, euler_p;

		sensor_sample_fetch_chan(dev, SENSOR_CHAN_YRP);
		sensor_channel_get(dev, SENSOR_CHAN_YAW, &euler_h);
		sensor_channel_get(dev, SENSOR_CHAN_ROLL, &euler_r);
		sensor_channel_get(dev, SENSOR_CHAN_PITCH, &euler_p);

		printk("Euler angles, H: %d.%06d R: %d.%06d P: %d.%06d\n",
		       euler_h.val1, euler_h.val2,
		       euler_r.val1, euler_r.val2,
		       euler_p.val1, euler_p.val2);

		k_sleep(K_MSEC(10));
	}
}
