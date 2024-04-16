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

#ifdef CONFIG_I2C_DUMP_MESSAGES_ALLOWLIST
#define DEF_BUS_WITH_ADDR(node, prop, idx) I2C_DT_SPEC_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),
#define DEF_ALLOWLIST_DEV(node)            DT_FOREACH_PROP_ELEM(node, devices, DEF_BUS_WITH_ADDR)

struct i2c_dt_spec messages_allowlist[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_i2c_dump_allowlist, DEF_ALLOWLIST_DEV)};

#undef DEF_ALLOWLIST_DEV
#undef DEF_BUS_WITH_ADDR
#endif

void i2c_dump_msgs_rw(const struct device *dev, const struct i2c_msg *msgs, uint8_t num_msgs,
		      uint16_t addr, bool dump_read)
{
#ifdef CONFIG_I2C_DUMP_MESSAGES_ALLOWLIST
	bool found_dev = 0;

	for (int a = 0; a < ARRAY_SIZE(messages_allowlist); a++) {
		struct i2c_dt_spec *allowed = &messages_allowlist[a];

		if (dev != allowed->bus || addr != allowed->addr) {
			continue;
		} else {
			found_dev = 1;
			break;
		}
	}

	if (!found_dev) {
		return;
	}
#endif

	LOG_DBG("I2C msg: %s, addr=%x", dev->name, addr);
	for (unsigned int i = 0; i < num_msgs; i++) {
		const struct i2c_msg *msg = &msgs[i];
		const bool is_read = msg->flags & I2C_MSG_READ;
		const bool dump_data = dump_read || !is_read;

		if (msg->len == 1 && dump_data) {
			LOG_DBG("   %c %2s %1s len=01: %02x", is_read ? 'R' : 'W',
				msg->flags & I2C_MSG_RESTART ? "Sr" : "",
				msg->flags & I2C_MSG_STOP ? "P" : "", msg->buf[0]);
		} else {
			LOG_DBG("   %c %2s %1s len=%02x: ", is_read ? 'R' : 'W',
				msg->flags & I2C_MSG_RESTART ? "Sr" : "",
				msg->flags & I2C_MSG_STOP ? "P" : "", msg->len);
			if (dump_data) {
				LOG_HEXDUMP_DBG(msg->buf, msg->len, "contents:");
			}
		}
	}
}
