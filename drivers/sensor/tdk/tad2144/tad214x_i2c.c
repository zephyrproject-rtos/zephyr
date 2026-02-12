/*
 * Copyright (c) 2025 TDK Invensense
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

uint8_t write_buf[257];
uint8_t read_buf[257];

static int tad214x_bus_check_i2c(const union tad214x_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int tad214x_read_reg_i2c(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				 uint32_t size)
{
	struct i2c_msg msg[2];
    int rc = 0;

    write_buf[0] = reg;
    write_buf[1] = crc8_sae_j1850(&reg, 1);

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = 2;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = size*2+1;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	rc = i2c_transfer(bus->i2c.bus, msg, 2, bus->i2c.addr);

    if (crc8_sae_j1850(&read_buf[0], size * 2) != read_buf[size*2]) {
         LOG_ERR("tad214x_read_reg_i2c rc: %d, crc: %x %x", 
             rc, crc8_sae_j1850(&read_buf[0], size * 2), read_buf[size*2]);
         return -EBADMSG;
     }

    memswap16(&read_buf[0], size*2);
    memcpy((uint8_t *) buf, read_buf, size*2);

    return rc;
}

static int tad214x_write_reg_i2c(const union tad214x_bus *bus, uint8_t reg, uint16_t *buf,
				  uint32_t size)
{
  	struct i2c_msg msg[2];
    int rc = 0;
    
    write_buf[0] = reg;
    memcpy(&write_buf[1],(uint8_t *)buf, size*2);

    memswap16(&write_buf[1], size*2);
    write_buf[size*2+1] = crc8_sae_j1850(write_buf, size*2+1);
    
	msg[0].buf = &write_buf[0];
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = &write_buf[1];
	msg[1].len = size*2+1;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	rc = i2c_transfer(bus->i2c.bus, msg, 2, bus->i2c.addr);

    return rc;
}

const struct tad214x_bus_io tad214x_bus_io_i2c = {
	.check = tad214x_bus_check_i2c,
	.read = tad214x_read_reg_i2c,
	.write = tad214x_write_reg_i2c,
};
#endif /* CONFIG_I2C */

