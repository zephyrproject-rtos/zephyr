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

static void on_changed(const struct device *dev)
{
	unsigned int size, i;
	uint8_t *data;

	size = eeprom_target_get_size(dev);
	data = eeprom_target_get_data(dev);

	printk("Eeprom changed, now contains:\n");
	for (i = 0; i < size; i++) {
		char sep = i % 16 == 15 ? '\n' : ' ';

		printk("%02x%c", data[i], sep);
	}
	printk("\n");
}

int main(void)
{
	printk("i2c target sample\n");

	if (!device_is_ready(eeprom)) {
		printk("eeprom device not ready\n");
		return 0;
	}

	eeprom_target_set_changed_callback(eeprom, on_changed);

	if (i2c_target_driver_register(eeprom) < 0) {
		printk("Failed to register i2c target driver\n");
		return 0;
	}

	printk("i2c target driver registered\n");

	return 0;
}
