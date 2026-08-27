/*
 * Copyright (c) 2023 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/printk.h>

/* getting all devices from device tree with alias "prox_sensor?" */
#define PROX_ALIASES(i) DT_ALIAS(_CONCAT(prox_sensor, i))
#define PROX_DEVICES(i, _) \
	IF_ENABLED(DT_NODE_EXISTS(PROX_ALIASES(i)),\
	(DEVICE_DT_GET(PROX_ALIASES(i)),))

/* creating a list with the first 10 devices */
static const struct device *prox_devices[] = {
	LISTIFY(10, PROX_DEVICES, ())
};

void print_prox_data(void)
{
	struct sensor_value pdata;

	for (int i = 0; i < ARRAY_SIZE(prox_devices); i++) {
		const struct device *dev = prox_devices[i];

		if (sensor_sample_fetch(dev)) {
			printk("Failed to fetch sample from %s\n", dev->name);
		}

		sensor_channel_get(dev, SENSOR_CHAN_PROX, &pdata);
		printk("Proximity on %s: %d\n", dev->name, pdata.val1);
	}
}

int main(void)
{
	printk("Proximity sensor sample application\n");
	printk("Found %d proximity sensor(s): ", ARRAY_SIZE(prox_devices));
	for (int i = 0; i < ARRAY_SIZE(prox_devices); i++) {
		printk("%s ", prox_devices[i]->name);
	}
	printk("\n");

	while (true) {
		k_sleep(K_MSEC(2000));
		print_prox_data();
	}
	return 0;
}
