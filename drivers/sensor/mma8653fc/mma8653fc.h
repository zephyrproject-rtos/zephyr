/*
 * Copyright (c) 2018 William Fish (Manaulytica Ltd)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MMA8653_MMA8653_H_
#define ZEPHYR_DRIVERS_SENSOR_MMA8653_MMA8653_H_

#include <device.h>
#include <misc/util.h>

/**
  * MMA8653 Register map (partial)
  */
#define MMA8653_STATUS          0x00
#define MMA8653_WHOAMI          0x0D
#define MMA8653_XYZ_DATA_CFG    0x0E
#define MMA8653_CTRL_REG1       0x2A
#define MMA8653_CTRL_REG2       0x2B
#define MMA8653_CTRL_REG3       0x2C
#define MMA8653_CTRL_REG4       0x2D
#define MMA8653_CTRL_REG5       0x2E

#define MMA8653_OUT_X_MSB		0x01
#define MMA8653_OUT_Y_MSB		0x03
#define MMA8653_OUT_Z_MSB		0x05

#define MMA8653_NUM_ACCEL_CHANNELS	3

enum MMA8653_channel {
	MMA8653_CHANNEL_ACCEL_X	= 0,
	MMA8653_CHANNEL_ACCEL_Y,
	MMA8653_CHANNEL_ACCEL_Z,
};

enum MMA8653_range {
	MMA8653_RANGE_2G	= 0,
	MMA8653_RANGE_4G,
	MMA8653_RANGE_8G,
};

struct MMA8653_config {
	char *i2c_name;
	u8_t i2c_address;
	u8_t whoami;
	enum mma8653_range range;
};

struct MMA8653_data {
	struct device *i2c;
	s16_t x;
	s16_t y;
	s16_t z;
};

#define SYS_LOG_DOMAIN "MMA8653"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_MMA8653_ */
