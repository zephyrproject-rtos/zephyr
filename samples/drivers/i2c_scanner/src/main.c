/*
 * Copyright (c) 2018 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/i2c.h>

#ifdef ARDUINO_I2C_LABEL
#define I2C_DEV ARDUINO_I2C_LABEL
#else
#define I2C_DEV "I2C_0"
#endif

/**
 * @file This app scans I2C bus for any devices present
 */

void main(void)
{
	struct device *i2c_dev;
	u8_t cnt = 0, first = 0x04, last = 0x77;

	printk("Starting i2c scanner...\n\n");

	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver %s not found.\n", I2C_DEV);
		return;
	}

	printk("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (u8_t i = 0; i <= last; i += 16) {
		printk("%02x: ", i);
		for (u8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				printk("   ");
				continue;
			}

			struct i2c_msg msgs[1];
			u8_t dst;

			/* Send the address to read from */
			msgs[0].buf = &dst;
			msgs[0].len = 0U;
			msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
			if (i2c_transfer(i2c_dev, &msgs[0], 1, i + j) == 0) {
				printk("%02x ", i + j);
				++cnt;
			} else {
				printk("-- ");
			}
		}
		printk("\n");
	}

	printk("\n%u devices found on %s\n", cnt, I2C_DEV);

}
