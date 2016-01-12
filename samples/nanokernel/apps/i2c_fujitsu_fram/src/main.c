/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <i2c.h>

/**
 * @file Sample app using the Fujitsu MB85RC256V FRAM through ARC I2C.
 */

#define FRAM_I2C_ADDR	0x50

int write_byte(struct device *i2c_dev, uint16_t addr, uint8_t data)
{
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* FRAM address */
	wr_addr[0] = (addr >> 8) & 0xFF;
	wr_addr[1] = addr & 0xFF;

	/* Setup I2C messages */

	/* Send the address to write */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Data to be written, and STOP after this. */
	msgs[1].buf = &data;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, FRAM_I2C_ADDR);
}

int read_byte(struct device *i2c_dev, uint16_t addr, uint8_t *data)
{
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* Now try to read back from FRAM */

	/* FRAM address */
	wr_addr[0] = (addr >> 8) & 0xFF;
	wr_addr[1] = addr & 0xFF;

	/* Setup I2C messages */

	/* Send the address to read */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. RESTART as neededm and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_RESTART;

	return i2c_transfer(i2c_dev, &msgs[0], 2, FRAM_I2C_ADDR);
}

void main(void)
{
	struct device *i2c_dev;
	uint8_t data;
	int ret;

#ifdef CONFIG_ARC
	i2c_dev = device_get_binding(CONFIG_I2C_QUARK_SE_SS_0_NAME);
#elif CONFIG_X86
	i2c_dev = device_get_binding(CONFIG_I2C_DW_0_NAME);
#endif
	if (!i2c_dev) {
		printk("I2C: Device not found.\n");
	}

	ret = write_byte(i2c_dev, 0x00, 0xAE);
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Wrote 0xAE to address 0x00.\n");
	}

	ret = write_byte(i2c_dev, 0x01, 0x86);
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Wrote 0x86 to address 0x01.\n");
	}

	ret = read_byte(i2c_dev, 0x00, &data);
	if (ret) {
		printk("Error reading to FRAM! (%d)\n", ret);
	} else {
		printk("Read 0x%X from address 0x00.\n", data);
	}

	ret = read_byte(i2c_dev, 0x01, &data);
	if (ret) {
		printk("Error reading to FRAM! (%d)\n", ret);
	} else {
		printk("Read 0x%X from address 0x01.\n", data);
	}
}
