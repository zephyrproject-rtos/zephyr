/*
 * Copyright (c) 2018 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <i2c.h>

#define I2C_DEV "I2C_0"

/**
 * @file This app scans I2C bus for any devices present
 */

void main(void)
{
	struct device *i2c_dev;

	printk("Starting i2c scanner...\n");

	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");
		return;
	}

	for (u8_t i = 4; i < 0x77; i++) {
		struct i2c_msg msgs[1];

		/* Send the address to read from */
		msgs[0].buf = NULL;
		msgs[0].len = 0;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		if (i2c_transfer(i2c_dev, &msgs[0], 1, i) == 0) {
			printk("0x%2x FOUND\n", i);
		}
	}
}
