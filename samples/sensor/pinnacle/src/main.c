/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

static struct sensor_trigger trackpad_trig;

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	struct sensor_value value[3];

	int rc = sensor_sample_fetch(dev);

	if (rc != 0) {
		printk("Error: failed to fetch sample");
		return;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_POS_XYZ, value);
	if (rc != 0) {
		printk("Error: failed to get sample");
		return;
	}

	printk("%u:%u:%u\n", (uint16_t)value[0].val1, (uint16_t)value[1].val1,
	       (uint16_t)value[2].val1);
}

int main(void)
{
	const struct device *trackpad = DEVICE_DT_GET_ONE(cirque_pinnacle);

	if (!device_is_ready(trackpad)) {
		printk("Error: failed to init trackpad\n");
		return 0;
	}

	trackpad_trig.type = SENSOR_TRIG_DATA_READY;
	trackpad_trig.chan = SENSOR_CHAN_POS_XYZ;

	int rc = sensor_trigger_set(trackpad, &trackpad_trig, trigger_handler);

	if (rc) {
		printk("Error: failed to set trackpad trigger\n");
	}

	return 0;
}
