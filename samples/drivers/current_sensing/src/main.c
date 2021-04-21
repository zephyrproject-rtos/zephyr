/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <sys/printk.h>

#include <device.h>
#include <drivers/i2c.h>

/**
 * @file Sample app using the TI INA219 through I2C.
 */

#define I2C_SLV_ADDR	0x40

/* The calibration value is based on components on
 * Adafruit's breakout board
 * (https://www.adafruit.com/products/904), where
 * the current sensing resistor is 0.1 ohm.
 * This enables measurements up to 32V, 2A.
 */
#define CAL_VAL		(4096)

/* With default calibration above,
 * Each current LSB is 100 uA == 0.1 mA == 0.0001 A.
 * Each power LSB is 2000 uW == 2 mW = 0.002W.
 */
#define CUR_LSB		100
#define PWR_LSB		2000

int read_reg16(const struct device *i2c_dev, uint8_t reg_addr,
	       uint8_t *data)
{
	uint8_t wr_addr;
	struct i2c_msg msgs[2];

	/* Register address */
	wr_addr = reg_addr;

	/* Setup I2C messages */

	/* Send the address to read */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. RESTART as neededm and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = 2U;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, I2C_SLV_ADDR);
}

int write_reg16(const struct device *i2c_dev, uint8_t reg_addr,
		uint8_t *data)
{
	uint8_t wr_addr;
	struct i2c_msg msgs[2];

	/* Register address */
	wr_addr = reg_addr;

	/* Setup I2C messages */

	/* Send the address to read */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. RESTART as neededm and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = 2U;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, I2C_SLV_ADDR);
}

void main(void)
{
	const struct device *i2c_dev;
	uint8_t data[2];
	uint32_t shunt_volt, bus_volt, current, power;

	i2c_dev = device_get_binding("I2C_0");
	if (!i2c_dev) {
		printk("I2C: Device not found.\n");
		return;
	}

	/* Configure the chip using default values */
	data[0] = 0x03;
	data[1] = 0x99;
	write_reg16(i2c_dev, 0x00, data);

	/* Write the calibration value */
	data[0] = (CAL_VAL & 0xFF00) >> 8;
	data[1] = CAL_VAL & 0xFF;
	write_reg16(i2c_dev, 0x05, data);

	/* Read bus voltage */
	read_reg16(i2c_dev, 0x02, data);
	bus_volt = (data[0] << 8) | data[1];
	bus_volt >>= 3; /* 3 LSBs are not data */
	bus_volt *= 4U; /* each LSB is 4 mV */
	printk("Bus Voltage: %d mV\n", bus_volt);

	/* Read shunt voltage */
	read_reg16(i2c_dev, 0x01, data);
	shunt_volt = (data[0] << 8) | data[1];
	shunt_volt *= 10U; /* to uV since each LSB is 0.01 mV */
	printk("Shunt Voltage: %d uV\n", shunt_volt);

	/* Read current */
	read_reg16(i2c_dev, 0x04, data);
	current = (data[0] << 8) | data[1];
	current *= CUR_LSB;
	printk("Current: %d uA\n", current);

	/* Read power  */
	read_reg16(i2c_dev, 0x03, data);
	power = (data[0] << 8) | data[1];
	power *= PWR_LSB;
	printk("Power: %d uW\n", power);
}
