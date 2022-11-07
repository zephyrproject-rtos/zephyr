/*
 * Logging of I2C messages
 *
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/drivers/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c);

#if defined(CONFIG_I2C_CALLBACK) && defined(CONFIG_POLL)
void z_i2c_transfer_signal_cb(const struct device *dev,
	int result,
	void *data)
{
	struct k_poll_signal *sig = (struct k_poll_signal *)data;

	k_poll_signal_raise(sig, result);
}
#endif

void i2c_dump_msgs(const char *name, const struct i2c_msg *msgs,
		   uint8_t num_msgs, uint16_t addr)
{
	LOG_DBG("I2C msg: %s, addr=%x", name, addr);
	for (unsigned int i = 0; i < num_msgs; i++) {
		const struct i2c_msg *msg = &msgs[i];

		LOG_DBG("   %c %s%s len=%02x: ",
			msg->flags & I2C_MSG_READ ? 'R' : 'W',
			msg->flags & I2C_MSG_RESTART ? "Sr " : "",
			msg->flags & I2C_MSG_STOP ? "P" : "",
			msg->len);
		if (!(msg->flags & I2C_MSG_READ)) {
			LOG_HEXDUMP_DBG(msg->buf, msg->len, "contents:");
		}
	}
}
