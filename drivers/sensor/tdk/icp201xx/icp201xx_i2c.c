/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICP201XX accessed via I2C.
 */

#include "icp201xx_drv.h"

static int icp201xx_read_reg_i2c(const union icp201xx_bus *bus, uint8_t reg, uint8_t *rbuffer,
				 uint32_t rlen)
{

	return i2c_burst_read_dt(&bus->i2c, reg, (uint8_t *)rbuffer, rlen);
}

static int icp201xx_write_reg_i2c(const union icp201xx_bus *bus, uint8_t reg, uint8_t *wbuffer,
				  uint32_t wlen)
{
	return i2c_burst_write_dt(&bus->i2c, reg, wbuffer, wlen);
}

const struct icp201xx_bus_io icp201xx_bus_io_i2c = {
	.read = icp201xx_read_reg_i2c,
	.write = icp201xx_write_reg_i2c,
};
