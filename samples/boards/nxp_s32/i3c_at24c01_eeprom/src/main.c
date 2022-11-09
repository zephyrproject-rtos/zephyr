/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/device.h>
#include <string.h>

/**
 * @file Sample app using the Microchip AT24C01C-XHM EEPROM through I3C driver.
 */

#define I3C_DEV_NODE	DT_NODELABEL(i3c2)
#define I2C_SLAVE_ADDR	0x50

/* Starting EEP address for writing to / reading from */
#define EEP_START_ADDR 0x10
#define NUM_BYTES	20

uint8_t read_data[NUM_BYTES];

#define DATA_TO_WRITE(n, _) (n + 1)

uint8_t data_to_write[NUM_BYTES] = {
	LISTIFY(NUM_BYTES, DATA_TO_WRITE, (,))
};

static int write_bytes(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t num_bytes)
{
	uint8_t i, curr_addr;
	int err;

	uint8_t write_buf[2];
	struct i2c_msg msg[2];

	err = 0;
	curr_addr = addr;

	for (i = 0; i < num_bytes; i++) {

		write_buf[0] = curr_addr;
		write_buf[1] = data[i];

		msg[0].buf = write_buf;
		msg[0].len = 2;
		msg[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		err = i2c_transfer(dev, msg, 1, I2C_SLAVE_ADDR);
		if (err) {
			break;
		}

		curr_addr++;

		k_sleep(K_MSEC(2));
	}

	return err;
}

static int read_bytes(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t num_bytes)
{
	uint8_t i, curr_addr;
	int err;
	struct i2c_msg msg[2];

	err = 0;
	curr_addr = addr;

	/* Read data */
	for (i = 0; i < num_bytes; i++) {

		/* Send a byte that contain the memory address will be read */
		msg[0].buf = &curr_addr;
		msg[0].len = 1;
		msg[0].flags = I2C_MSG_WRITE;

		msg[1].buf = &data[i];
		msg[1].len = 1;
		msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

		err = i2c_transfer(dev, msg, 2, I2C_SLAVE_ADDR);
		if (err) {
			break;
		}

		curr_addr++;

		k_sleep(K_MSEC(2));
	}

	return err;
}

void main(void)
{
	const struct device *i3c_dev;
	int err;

	i3c_dev = DEVICE_DT_GET(I3C_DEV_NODE);

	if (!i3c_dev) {
		printk("Cannot get device\n");
		return;
	}

	if (!device_is_ready(i3c_dev)) {
		printk("Device is not ready\n");
		return;
	}

	err = write_bytes(i3c_dev, EEP_START_ADDR, &data_to_write[0], 1);
	if (err) {
		printk("Error writing a byte to EEPROM, error code (%d)\n", err);
		return;
	}

	printk("Wrote 0x%X to address 0x%X\n", data_to_write[0], EEP_START_ADDR);

	err = read_bytes(i3c_dev, EEP_START_ADDR, &read_data[0], 1);
	if (err) {
		printk("Error reading a byte from EEPROM, error code (%d)\n", err);
		return;
	}

	printk("Read 0x%X from address 0x%X\n", read_data[0], EEP_START_ADDR);

	/* Compare data */
	if (memcmp(read_data, data_to_write, 1)) {
		printk("Sent and received data is not the same");
		return;
	}

	err = write_bytes(i3c_dev, EEP_START_ADDR, data_to_write, NUM_BYTES);
	if (err) {
		printk("Error writing %d bytes to EEPROM, error code (%d)\n", NUM_BYTES, err);
		return;
	}

	printk("Wrote %d bytes to address starting from 0x%X\n",
			NUM_BYTES, EEP_START_ADDR);

	err = read_bytes(i3c_dev, EEP_START_ADDR, read_data, NUM_BYTES);
	if (err) {
		printk("Error reading %d bytes from EEPROM! error code (%d)\n", NUM_BYTES, err);
		return;
	}

	printk("Read %d bytes from starting address 0x%X\n",
			NUM_BYTES, EEP_START_ADDR);

	/* Compare data */
	if (memcmp(read_data, data_to_write, NUM_BYTES)) {
		printk("Sent and received data is not the same");
		return;
	}

	printk("EEPROM read/write successful\n");
}
