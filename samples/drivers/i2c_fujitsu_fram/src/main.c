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

#include <errno.h>

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <i2c.h>

/**
 * @file Sample app using the Fujitsu MB85RC256V FRAM through ARC I2C.
 */

#define FRAM_I2C_ADDR	0x50

int write_bytes(struct device *i2c_dev, uint16_t addr,
		uint8_t *data, uint32_t num_bytes)
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
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, FRAM_I2C_ADDR);
}

int read_bytes(struct device *i2c_dev, uint16_t addr,
	       uint8_t *data, uint32_t num_bytes)
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
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, FRAM_I2C_ADDR);
}

void main(void)
{
	struct device *i2c_dev;
	uint8_t cmp_data[16];
	uint8_t data[16];
	int i, ret;

	i2c_dev = device_get_binding("I2C_0");
	if (!i2c_dev) {
		printk("I2C: Device not found.\n");
	}

	/* Do one-byte read/write */

	data[0] = 0xAE;
	ret = write_bytes(i2c_dev, 0x00, &data[0], 1);
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Wrote 0xAE to address 0x00.\n");
	}

	data[0] = 0x86;
	ret = write_bytes(i2c_dev, 0x01, &data[0], 1);
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Wrote 0x86 to address 0x01.\n");
	}

	data[0] = 0x00;
	ret = read_bytes(i2c_dev, 0x00, &data[0], 1);
	if (ret) {
		printk("Error reading to FRAM! (%d)\n", ret);
	} else {
		printk("Read 0x%X from address 0x00.\n", data[0]);
	}

	data[1] = 0x00;
	ret = read_bytes(i2c_dev, 0x01, &data[0], 1);
	if (ret) {
		printk("Error reading to FRAM! (%d)\n", ret);
	} else {
		printk("Read 0x%X from address 0x01.\n", data[0]);
	}

	/* Do multi-byte read/write */

	/* get some random data, and clear out data[] */
	for (i = 0; i < sizeof(cmp_data); i++) {
		cmp_data[i] = sys_cycle_get_32() & 0xFF;
		data[i] = 0x00;
	}

	/* write them to the FRAM */
	ret = write_bytes(i2c_dev, 0x00, cmp_data, sizeof(cmp_data));
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Wrote %zu bytes to address 0x00.\n", sizeof(cmp_data));
	}

	ret = read_bytes(i2c_dev, 0x00, data, sizeof(data));
	if (ret) {
		printk("Error writing to FRAM! (%d)\n", ret);
	} else {
		printk("Read %zu bytes from address 0x00.\n", sizeof(data));
	}

	ret = 0;
	for (i = 0; i < sizeof(cmp_data); i++) {
		/* uncomment below if you want to see all the bytes */
		/* printk("0x%X ?= 0x%X\n", cmp_data[i], data[i]); */
		if (cmp_data[i] != data[i]) {
			printk("Data comparison failed @ %d.\n", i);
			ret = -EIO;
		}
	}
	if (ret == 0) {
		printk("Data comparison successful.\n");
	}
}
