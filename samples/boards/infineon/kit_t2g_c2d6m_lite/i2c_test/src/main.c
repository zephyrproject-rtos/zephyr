/*
 * Copyright (c) 2025 Linumiz GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

/* Get I2C device */
#define I2C_DEV_NODE DT_NODELABEL(scb11)
/**
 * @brief Scan I2C bus for devices (using 1-byte read method)
 */
static void i2c_scan_bus(const struct device *i2c_dev)
{
	uint8_t dummy;
	int devices_found = 0;

	printk("\nI2C Bus Scan\n");
	printk("Scanning I2C bus...\n");

	for (uint16_t addr = 0x03; addr < 0x78; addr++) {
		/* Try to read 1 byte (most I2C devices support this) */
		int ret = i2c_read(i2c_dev, &dummy, 1, addr);

		if (ret == 0) {
			printk("  Device found at address 0x%02X\n", addr);
			devices_found++;
		}
	}

	if (devices_found == 0) {
		printk("No devices found\n");
	} else {
		printk("Total devices found: %d\n", devices_found);
	}
	printk("Scan Complete\n\n");
}

int main(void)
{
	const struct device *i2c_dev;
	int ret;

	printk("Infineon CYT4DN I2C Driver Test\n");

	/* Get I2C device */
	i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	if (!device_is_ready(i2c_dev)) {
		printk("ERROR: I2C device not ready!\n");
		return -1;
	}
	printk("I2C device ready: %s\n", i2c_dev->name);

	/* Configure I2C to 100kHz standard mode */
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	ret = i2c_configure(i2c_dev, i2c_cfg);
	if (ret < 0) {
		printk("ERROR: Failed to configure I2C (err %d)\n", ret);
		return -1;
	}
	printk("I2C configured: 100 kHz Standard Mode\n");

	/* Scan I2C bus */
	i2c_scan_bus(i2c_dev);

	return 0;
}
