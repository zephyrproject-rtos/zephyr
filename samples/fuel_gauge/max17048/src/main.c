/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/fuel_gauge.h>



int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(maxim_max17048);
	int ret = 0;

	if (dev == NULL) {
		printk("\nError: no device found.\n");
		return 0;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return 0;
	}



	printk("Found device \"%s\", getting fuel gauge data\n", dev->name);

	if (dev == NULL) {
		return 0;
	}

	while (1) {

		struct fuel_gauge_get_property props[] = {
			{
				.property_type = FUEL_GAUGE_RUNTIME_TO_EMPTY,
			},
			{
				.property_type = FUEL_GAUGE_RUNTIME_TO_FULL,
			},
			{
				.property_type = FUEL_GAUGE_STATE_OF_CHARGE,
			},
			{
				.property_type = FUEL_GAUGE_VOLTAGE,
			}
		};

		ret = fuel_gauge_get_prop(dev, props, ARRAY_SIZE(props));
		if (ret < 0) {
			printk("Error: cannot get properties\n");
		} else {
			if (ret != 0) {
				printk("Warning: Some properties failed\n");
			}

			if (props[0].status == 0) {
				printk("Time to empty %d\n", props[0].value.runtime_to_empty);
			} else {
				printk(
				"Property FUEL_GAUGE_RUNTIME_TO_EMPTY failed with error %d\n",
				props[0].status
				);
			}

			if (props[1].status == 0) {
				printk("Time to full %d\n", props[1].value.runtime_to_full);
			} else {
				printk(
				"Property FUEL_GAUGE_RUNTIME_TO_FULL failed with error %d\n",
				props[1].status
				);
			}

			if (props[2].status == 0) {
				printk("Charge %d%%\n", props[2].value.state_of_charge);
			} else {
				printk(
				"Property FUEL_GAUGE_STATE_OF_CHARGE failed with error %d\n",
				props[2].status
				);
			}

			if (props[3].status == 0) {
				printk("Voltage %d\n", props[3].value.voltage);
			} else {
				printk(
				"Property FUEL_GAUGE_VOLTAGE failed with error %d\n",
				props[3].status
				);
			}
		}


		k_sleep(K_MSEC(5000));
	}
	return 0;
}
