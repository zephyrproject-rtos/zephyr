/*
 * Logging of I2C messages
 *
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <drivers/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c);

void i2c_dump_msgs(const char *name, const struct i2c_msg *msgs,
		   uint8_t num_msgs, uint16_t addr)
{
	LOG_DBG("I2C msg: %s, addr=%x", name, addr);
	for (unsigned int i = 0; i < num_msgs; i++) {
		const struct i2c_msg *msg = &msgs[i];

		LOG_DBG("   %c len=%02x: ",
			msg->flags & I2C_MSG_READ ? 'R' : 'W', msg->len);
		if (!(msg->flags & I2C_MSG_READ)) {
			LOG_HEXDUMP_DBG(msg->buf, msg->len, "contents:");
		}
	}
}
