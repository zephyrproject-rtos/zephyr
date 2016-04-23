/*
 * Copyright (c) 2016 Intel Corporation
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

#define GYRO_I2C_ADDR 0x6A
#define WHO_AM_I_REG 0x0F

static void read_who_am_i(struct device *dev)
{
	uint8_t data;

	data = WHO_AM_I_REG;
	if (i2c_write(dev, &data, sizeof(data), GYRO_I2C_ADDR) != 0) {
		printk("Error on i2c_write()\n");
		return;
	}

	data = 0;
	if (i2c_read(dev, &data, sizeof(data), GYRO_I2C_ADDR) != 0) {
		printk("Error on i2c_read()\n");
		return;
	}

	if (data == 0xd4)
		printk("Register Who am I read successfully\n");
	else
		printk("Register Who am I read FAILED\n");
}

static void read_who_am_i_by_transfer(struct device *dev)
{
	struct i2c_msg msg[2];
	uint8_t write_data = WHO_AM_I_REG;
	uint8_t read_data = 0;

	msg[0].flags = I2C_MSG_WRITE | I2C_MSG_RESTART;
	msg[0].buf = &write_data;
	msg[0].len = sizeof(write_data);

	msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP;
	msg[1].buf = &read_data;
	msg[1].len = sizeof(read_data);

	if (i2c_transfer(dev, msg, 2, GYRO_I2C_ADDR) != 0) {
		printk("Error on i2c_transfer()\n");
		return;
	}

	if (read_data == 0xd4)
		printk("Register Who am I read successfully\n");
	else
		printk("Register Who am I read FAILED\n");
}

void main(void)
{
	union dev_config cfg;
	struct device *dev;

	printk("Start I2C LSM9DS0 sample\n");

	cfg.raw = 0;
	cfg.bits.use_10_bit_addr = 0;
	cfg.bits.speed = I2C_SPEED_STANDARD;
	cfg.bits.is_master_device = 1;

	dev = device_get_binding("I2C_0");
	if (!dev) {
		printk("I2C0: Device not found.\n");
		return;
	}

	if (i2c_configure(dev, cfg.raw) != 0) {
		printk("Error on i2c_configure()\n");
		return;
	}

	read_who_am_i(dev);
	read_who_am_i_by_transfer(dev);
}
