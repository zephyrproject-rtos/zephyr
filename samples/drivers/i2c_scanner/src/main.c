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
#include <generated_dts_board.h>

#ifdef ARDUINO_I2C_LABEL
#define I2C_DEV ARDUINO_I2C_LABEL
#else
#define I2C_DEV "I2C_0"
#endif

#define device_get_dt_binding_zdid(zdid) _CONCAT(&__device_, zdid)

#define device_get_dt_binding(name, inst) \
device_get_dt_binding_zdid(DT_ZDID_INST_##inst##_##name) \

/**
 * @file This app scans I2C bus for any devices present
 */

void main(void)
{
	struct device *i2c_dev;

	printk("Starting i2c scanner...\n");

	i2c_dev = device_get_dt_binding(SILABS_GECKO_I2C, 0);
	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");
		return;
	}

	for (u8_t i = 4; i <= 0x77; i++) {
		struct i2c_msg msgs[1];
		u8_t dst;

		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 0U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		if (i2c_transfer(i2c_dev, &msgs[0], 1, i) == 0) {
			printk("0x%2x FOUND\n", i);
		}
	}
}
