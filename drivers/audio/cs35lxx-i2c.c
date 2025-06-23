/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C functions for Cirrus Logic CS35L family audio drivers
 */

#if CONFIG_AUDIO_CODEC_CS35LXX_I2C
#include "cs35lxx.h"

#include <zephyr/sys/byteorder.h>

static bool cs35lxx_is_ready_i2c(const union cs35lxx_bus *const bus)
{
	return i2c_is_ready_dt(&bus->i2c);
}

static const struct device *cs35lxx_get_device_i2c(const union cs35lxx_bus *const bus)
{
	return bus->i2c.bus;
}

static int cs35lxx_read_i2c(const union cs35lxx_bus *const bus, const uint32_t addr,
			    uint32_t *const rx, const uint32_t len)
{
	uint8_t addr_be32[CS35LXX_ADDRESS_WIDTH];
	int ret;

	sys_put_be32(addr, addr_be32);

	ret = i2c_write_read_dt(&bus->i2c, addr_be32, CS35LXX_ADDRESS_WIDTH, (uint8_t *)rx,
				len * CS35LXX_REGISTER_WIDTH);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t i = 0; i < len; i++) {
		rx[i] = sys_get_be32((uint8_t *)&rx[i]);
	}

	return 0;
}

static int cs35lxx_raw_write_i2c(const union cs35lxx_bus *const bus, const uint32_t addr,
				 const uint32_t *const tx, const uint32_t len)
{
	uint8_t addr_be32[CS35LXX_ADDRESS_WIDTH];
	struct i2c_msg msgs[2];
	int ret;

	sys_put_be32(addr, addr_be32);

	msgs[0].buf = addr_be32;
	msgs[0].len = CS35LXX_ADDRESS_WIDTH;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = (uint8_t *)tx;
	msgs[1].len = len * CS35LXX_REGISTER_WIDTH;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&bus->i2c, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int cs35lxx_write_i2c(const union cs35lxx_bus *const bus, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	for (uint32_t i = 0; i < len; i++) {
		sys_put_be32(tx[i], (uint8_t *)&tx[i]);
	}

	return cs35lxx_raw_write_i2c(bus, addr, tx, len);
}

const struct cs35lxx_io cs35lxx_io_i2c = {
	.is_ready = cs35lxx_is_ready_i2c,
	.get_device = cs35lxx_get_device_i2c,
	.read = cs35lxx_read_i2c,
	.write = cs35lxx_write_i2c,
	.raw_write = cs35lxx_raw_write_i2c,
};
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_I2C */
