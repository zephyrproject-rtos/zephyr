/*
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>

#define EEPROM_SAMPLE_OFFSET 0
#define EEPROM_SAMPLE_MAGIC  0xEE9703

struct perisistant_values {
	uint32_t magic;
	uint32_t boot_count;
};

/*
 * Get a device structure from a devicetree node with alias eeprom-0
 */
static const struct device *get_eeprom_device(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found EEPROM device \"%s\"\n", dev->name);
	return dev;
}

void main(void)
{
	const struct device *eeprom = get_eeprom_device();
	size_t eeprom_size;
	struct perisistant_values values;
	int rc;

	if (eeprom == NULL) {
		return;
	}

	eeprom_size = eeprom_get_size(eeprom);
	printk("Using eeprom with size of: %d.\n", eeprom_size);

	rc = eeprom_read(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
	if (rc < 0) {
		printk("Error: Couldn't read eeprom: err: %d.\n", rc);
		return;
	}

	if (values.magic != EEPROM_SAMPLE_MAGIC) {
		values.magic = EEPROM_SAMPLE_MAGIC;
		values.boot_count = 0;
	}

	values.boot_count++;
	printk("Device booted %d times.\n", values.boot_count);

	rc = eeprom_write(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
	if (rc < 0) {
		printk("Error: Couldn't write eeprom: err:%d.\n", rc);
		return;
	}

	printk("Reset the MCU to see the increasing boot counter.\n\n");
}
