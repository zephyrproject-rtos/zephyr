/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C functions for Cirrus Logic haptic drivers
 */

#if CONFIG_HAPTICS_CS40LXX_I2C
#include "cs40lxx.h"
#include <zephyr/sys/byteorder.h>

static bool cs40lxx_is_ready_i2c(const union cs40lxx_bus *const bus)
{
	return i2c_is_ready_dt(&bus->i2c);
}

static const struct device *cs40lxx_get_device_i2c(const union cs40lxx_bus *const bus)
{
	return bus->i2c.bus;
}

static int cs40lxx_read_i2c(const union cs40lxx_bus *const bus, const uint32_t addr,
			    uint32_t *const rx, const uint32_t len)
{
	uint8_t addr_be32[4];
	int ret;

	sys_put_be32(addr, addr_be32);

	ret = i2c_write_read_dt(&bus->i2c, addr_be32, CS40LXX_ADDRESS_WIDTH, (uint8_t *)rx,
				len * CS40LXX_REGISTER_WIDTH);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < len; i++) {
		rx[i] = sys_get_be32((uint8_t *)&rx[i]);
	}

	return 0;
}

static int cs40lxx_raw_write_i2c(const union cs40lxx_bus *const bus, const uint32_t addr,
				 const uint32_t *const tx, const uint32_t len)
{
	struct i2c_msg msgs[2];
	uint8_t addr_be32[4];
	int ret;

	sys_put_be32(addr, addr_be32);

	msgs[0].buf = addr_be32;
	msgs[0].len = CS40LXX_ADDRESS_WIDTH;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = (uint8_t *)tx;
	msgs[1].len = len * CS40LXX_REGISTER_WIDTH;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&bus->i2c, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int cs40lxx_write_i2c(const union cs40lxx_bus *const bus, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	for (int i = 0; i < len; i++) {
		sys_put_be32(tx[i], (uint8_t *)&tx[i]);
	}

	return cs40lxx_raw_write_i2c(bus, addr, tx, len);
}

const struct cs40lxx_io cs40lxx_io_i2c = {
	.is_ready = cs40lxx_is_ready_i2c,
	.get_device = cs40lxx_get_device_i2c,
	.read = cs40lxx_read_i2c,
	.write = cs40lxx_write_i2c,
	.raw_write = cs40lxx_raw_write_i2c,
};
#endif /* CONFIG_HAPTICS_CS40LXX_I2C */
