/*
 * Copyright (c) 2024 Open Pixel Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>

static const struct device *eeprom = DEVICE_DT_GET(DT_NODELABEL(eeprom0));

int main(void)
{
	printk("i2c target sample\n");

	if (!device_is_ready(eeprom)) {
		printk("eeprom device not ready\n");
		return 0;
	}

	if (i2c_target_driver_register(eeprom) < 0) {
		printk("Failed to register i2c target driver\n");
		return 0;
	}

	printk("i2c target driver registered\n");

	k_msleep(1000);

	if (i2c_target_driver_unregister(eeprom) < 0) {
		printk("Failed to unregister i2c target driver\n");
		return 0;
	}

	printk("i2c target driver unregistered\n");

	return 0;
}
