/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <drivers/fram.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/*
 * Get a device structure from a devicetree node with compatible
 * "fujitsu,frammb85rs64v". (If there are multiple, just pick one.)
 */
static const struct device *get_frammb85rs64v_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(fujitsu_frammb85rs64v);

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

int main(void)
{
	const struct device *dev = get_frammb85rs64v_device();

	if (dev == NULL) {
		return 0;
	}

	uint16_t addr = 0x0124;
	const uint8_t send_data[] = { 0xAA, 0x55, 0xAB };
	uint32_t len = sizeof(send_data) / sizeof(uint8_t);
	uint8_t receive_data[len];
	int err;
	err = fram_write(dev, addr, send_data, len);
	if (err) {
		printk("Error writing to FRAM! error code (%d)\n", err);
		return 0;
	} else {
		printk("Wrote %" PRIu32 " bytes to address 0x%X.\n", len, addr);
	}
	err = fram_read(dev, addr, receive_data, len);
	if (err) {
		printk("Error reading from FRAM! error code (%d)\n", err);
		return 0;
	} else {
		printk("Read %" PRIu32 " bytes to address 0x%X.\n", len, addr);
	}
	err = 0;
	for (int i = 0; i < len; i++) {
		if (send_data[i] != receive_data[i]) {
			printk("Data comparison failed @ %d.\n", i);
			err = -EIO;
		}
	}
	if (err == 0) {
		printk("Data comparison successful.\n");
	}

	return 0;
}
