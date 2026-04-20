/*
 * Copyright (c) 2026 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for TAD214x accessed via I2C.
 */

#include "tad214x_drv.h"

#if CONFIG_I2C

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TAD214X_I2C, CONFIG_SENSOR_LOG_LEVEL);

static int tad214x_bus_check_i2c(const union tad214x_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int tad214x_read_reg_i2c(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				 uint32_t size)
{
	uint8_t write_buf[TAD214X_BUFFER_LEN];
	uint8_t read_buf[TAD214X_BUFFER_LEN];

	if(size * 2 + 1 > TAD214X_BUFFER_LEN) {
		LOG_ERR("Size error: %d", size);
		return -EINVAL;
	}

	write_buf[0] = reg;
	write_buf[1] = crc8_sae_j1850(&reg, 1);

	i2c_write(bus->i2c.bus, write_buf, 2, bus->i2c.addr);
	i2c_read(bus->i2c.bus, read_buf, size * 2 + 1, bus->i2c.addr);

	if (crc8_sae_j1850(read_buf, size * 2) != read_buf[size * 2]) {
		LOG_ERR("CRC Mismatch! Calc: %x, Recv: %x",
			crc8_sae_j1850(read_buf, size * 2), read_buf[size * 2]);
		return -EBADMSG;
	}

	memswap16(read_buf, size * 2);
	memcpy((uint8_t *)buf, read_buf, size * 2);

	return 0;
}

static int tad214x_write_reg_i2c(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				  uint32_t size)
{
	struct i2c_msg msg;
	uint8_t write_buf[TAD214X_BUFFER_LEN];

	write_buf[0] = reg;
	memcpy(&write_buf[1], (uint8_t *)buf, size * 2);

	memswap16(&write_buf[1], size * 2);
	write_buf[size * 2 + 1] = crc8_sae_j1850(write_buf, size * 2 + 1);

	msg.buf = write_buf;
	msg.len = 1U + (size * 2) + 1U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	i2c_transfer(bus->i2c.bus, &msg, 1, bus->i2c.addr);
	return 0;
}

const struct tad214x_bus_io tad214x_bus_io_i2c = {
	.check = tad214x_bus_check_i2c,
	.read = tad214x_read_reg_i2c,
	.write = tad214x_write_reg_i2c,
};
#endif /* CONFIG_I2C */
