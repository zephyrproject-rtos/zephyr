/*
 * Copyright (C) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i3c.h>

#define I3C_DEV_PID                   0xFB1122330001

#define I2C_ADDR                      0x50
#define I2C_REG_ADDR                  0x00
#define MAX_BYTES                     31
#define MAX_BYTES_FOR_REGISTER_INDEX  1

static int i2c_write_bytes(const struct device *i2c_dev, uint8_t addr,
		       uint8_t *data, uint32_t num_bytes);

static int i2c_read_bytes(const struct device *i2c_dev, uint8_t addr,
		      uint8_t *data, uint32_t num_bytes);

int main(void)
{
	uint8_t data[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee};
	uint8_t rdata[5];
	int ret;

	const struct device *i3c_dev = DEVICE_DT_GET(DT_NODELABEL(i3c0));

	if (!device_is_ready(i3c_dev)) {
		printk("I3C: Device is not ready\n");
		return -ENODEV;
	}

	/* I3C write-read operation */
	printk("\n#### Starting i3c read-write transactions ####\n");
	uint64_t pid = I3C_DEV_PID;
	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *target = i3c_device_find(i3c_dev, &i3c_id);

	printk("%s: PID 0x%012llx\n", i3c_dev->name, pid);

	if (target != NULL) {
		/**
		 * dummy read, as in bus_init we write some data as a part of CCC,
		 * hence with this read transaction we are flushing the RX fifo.
		 * This is needed because the way simics i3c target is simulated
		 * it pops out the invert of the last written value.
		 */
		ret = i3c_read(target, &rdata[0], 1);

		ret = i3c_write(target, data, sizeof(data));
		if (ret) {
			printk("Error writing: Device address (%d), error code (%d)\n",
				target->dynamic_addr, ret);
			return ret;
		}
		printk("Wrote to i3c device with address (%d)\n",
			target->dynamic_addr);

		ret =  i3c_read(target, rdata, 5);
		if (ret) {
			printk("Error reading: Device address (%d), error code (%d)\n",
			target->dynamic_addr, ret);
			return ret;
		}

		printk("Read i3c device with address (%d)\n", target->dynamic_addr);

		printk("Read data: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n",
				rdata[0], rdata[1], rdata[2], rdata[3], rdata[4]);
	}

	/* I2C write-read operation */
	printk("\n#### Starting i2c read-write transactions ####\n");
	uint8_t i2c_data[] = {0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xb0, 0xb7};
	uint8_t i2c_dataRx[sizeof(i2c_data)];

	ret = i2c_write_bytes(i3c_dev, I2C_REG_ADDR, &i2c_data[0], sizeof(i2c_data));
	if (ret) {
		printk("Error writing to I2C Device! error code (%d)\n", ret);
		return ret;
	}
	printk("i2c write transaction successful\n");

	(void)memset(i2c_dataRx, 0, sizeof(i2c_dataRx));
	ret = i2c_read_bytes(i3c_dev, I2C_REG_ADDR, &i2c_dataRx[0], sizeof(i2c_dataRx));
	if (ret) {
		printk("Error reading an I2C Device! error code (%d)\n", ret);
		return ret;
	}

	if (memcmp(i2c_data, i2c_dataRx, sizeof(data)) != 0) {
		printk("i2c Data comparison not successful\n");
	} else {
		printk("i2c Data comparison successful\n");
	}

	return 0;
}

/* i2c write byte wrapper */
static int i2c_write_bytes(const struct device *i2c_dev, uint8_t addr,
		       uint8_t *data, uint32_t num_bytes)
{
	uint8_t wr_addr_data[MAX_BYTES + MAX_BYTES_FOR_REGISTER_INDEX - 1];
	struct i2c_msg msgs[1];
	int i = 0;

	wr_addr_data[0] = addr; /* FRAM address */

	/* load data */
	for (i = 0; i < num_bytes; i++) {
		wr_addr_data[MAX_BYTES_FOR_REGISTER_INDEX + i] = data[i];
	}

	/*incrementing because inner eeprom address is also of 1 byte*/
	uint32_t nbytes = num_bytes + 1;

	/* Setup I2C messages */
	/* Send the address + data to write to */
	msgs[0].buf = wr_addr_data;
	msgs[0].len = nbytes;
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 1, I2C_ADDR);
}

/* i2c read byte wrapper */
static int i2c_read_bytes(const struct device *i2c_dev, uint8_t addr,
		      uint8_t *data, uint32_t num_bytes)
{
	struct i2c_msg msgs[2];

	/* Setup I2C messages */

	/* Send the address to read from */
	msgs[0].buf = (uint8_t *)&addr; /* FRAM address */
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, I2C_ADDR);
}
