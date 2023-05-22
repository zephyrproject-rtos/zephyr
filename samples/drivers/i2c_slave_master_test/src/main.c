/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define I2C_SLAVE_ADDR 0x1EUL

int i2c_slave_write_requested_cb(struct i2c_target_config *config);
int i2c_slave_read_requested_cb(struct i2c_target_config *config, uint8_t *val);
int i2c_slave_write_received_cb(struct i2c_target_config *config, uint8_t val);
int i2c_slave_read_processed_cb(struct i2c_target_config *config, uint8_t *val);
int i2c_slave_stop_cb(struct i2c_target_config *config);
struct i2c_target_config i2c_data_cfg;
struct i2c_target_callbacks i2c_slave_cbs;

static int read_byte_from_slave(const struct device *i2c_dev, uint8_t *byte);
static int write_bytes_to_slave(const struct device *i2c_dev, uint8_t *p_to_slave_tx, uint16_t len);
static int read_bytes_from_slave(const struct device *i2c_dev, uint8_t *p_data);

uint8_t data_from_slave[16];
uint8_t test_data = 10;
uint8_t increment;

void main(void)
{
	int ret;
	uint8_t data_for_slave[3] = {0, 1, 2};
	uint8_t read_byte;

	i2c_slave_cbs.write_requested = i2c_slave_write_requested_cb;
	i2c_slave_cbs.read_requested = i2c_slave_read_requested_cb;
	i2c_slave_cbs.write_received = i2c_slave_write_received_cb;
	i2c_slave_cbs.read_processed = i2c_slave_read_processed_cb;
	i2c_slave_cbs.stop = i2c_slave_stop_cb;

	i2c_data_cfg.address = I2C_SLAVE_ADDR;
	i2c_data_cfg.callbacks = &i2c_slave_cbs;

	const struct device *dev_i2c_master = DEVICE_DT_GET(DT_ALIAS(i2c0));
	const struct device *dev_i2c_slave = DEVICE_DT_GET(DT_ALIAS(i2c1));

	printk("I2C Slave test\n");

	if (!device_is_ready(dev_i2c_master)) {
		printk("I2C Master Device is not ready.\n");
		return;
	}
	printk("I2C Master Device is ready\n");

	i2c_configure(dev_i2c_master, I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER);

	if (!device_is_ready(dev_i2c_slave)) {
		printk("I2C Slave Device is not ready.\n");
		return;
	}
	printk("I2C Slave Device is ready\n");

	ret = i2c_target_register(dev_i2c_slave, &i2c_data_cfg);
	if (ret) {
		printk("I2C Slave Error in configuring slave [ret %d]\n", ret);
		return;
	}


	while (1) {
		printk("=== READ BYTE ===\n");
		ret = read_byte_from_slave(dev_i2c_master, &read_byte);
		if (ret != 0) {
			printk("--> MSR: Error in reading byte: %d\n", ret);
		} else {
			printk("MSR RX: 0x%02X\n", read_byte);
		}

		printk("=== WRITE ===\n");
		for (int i = 0; i < sizeof(data_for_slave); i++) {
			data_for_slave[i] = data_for_slave[i] + 1;
		}
		ret = write_bytes_to_slave(dev_i2c_master, data_for_slave, sizeof(data_for_slave));
		if (ret != 0) {
			printk("--> MSR: Error in writing data: %d\n", ret);
		}

		printk("=== READ ===\n");
		if (read_bytes_from_slave(dev_i2c_master, data_from_slave) == 0) {
			for (uint8_t i = 0; i < 10; i++) {
				printk("MSR RX %d: 0x%02X\n", i, data_from_slave[i]);
			}
		} else {
			printk("--> MSR: Error in reading data\n");
		}

		k_msleep(10 * 1000);
	}
}

static int read_byte_from_slave(const struct device *i2c_dev, uint8_t *byte)
{
	struct i2c_msg msg;

	msg.buf = byte;
	msg.len = 1;
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, I2C_SLAVE_ADDR);
}

static int write_bytes_to_slave(const struct device *i2c_dev, uint8_t *p_to_slave_tx, uint16_t len)
{
	struct i2c_msg msg;

	msg.buf = p_to_slave_tx;
	msg.len = len;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, I2C_SLAVE_ADDR);
}

static int read_bytes_from_slave(const struct device *i2c_dev, uint8_t *p_data)
{
	uint8_t wr_addr = 0x0C;
	struct i2c_msg msgs[2];

	msgs[0].buf = &wr_addr;
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = p_data;
	msgs[1].len = 10;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, msgs, 2, I2C_SLAVE_ADDR);
}

int i2c_slave_write_requested_cb(struct i2c_target_config *config)
{
	printk("SLV: Wr Reg\n");
	return 0;
}

int i2c_slave_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
	printk("SLV RX: 0x%02X\n", val);
	return 0;
}

int i2c_slave_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
	*val = test_data++;
	increment = 0;
	printk("SLV TX %d: 0x%02X\n", increment, *val);
	return 0;
}

int i2c_slave_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
	*val = test_data + increment;
	increment++;
	printk("SLV TX %d: 0x%02X\n", increment, *val);
	return 0;
}

int i2c_slave_stop_cb(struct i2c_target_config *config)
{
	printk("SLV: Stop\n");
	return 0;
}
