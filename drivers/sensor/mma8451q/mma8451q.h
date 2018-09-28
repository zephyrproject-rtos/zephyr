/*
 * Copyright (c) 2018 Lars Knudsen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MMA8451Q_MMA8451Q_H_
#define ZEPHYR_DRIVERS_SENSOR_MMA8451Q_MMA8451Q_H_

#include <device.h>
#include <misc/util.h>

#define MMA8451Q_OUT_X_MSB		0x01
#define MMA8451Q_OUT_Y_MSB		0x03
#define MMA8451Q_OUT_Z_MSB		0x05

#define MMA8451Q_REG_WHOAMI		0x0d

#define MMA8451Q_XYZ_DATA_CFG		0x0E

#define MMA8451Q_CTRL_REG1		0x2A
#define MMA8451Q_CTRL_REG2		0x2B
#define MMA8451Q_CTRL_REG3		0x2C
#define MMA8451Q_CTRL_REG4		0x2D
#define MMA8451Q_CTRL_REG5		0x2E

#define MMA8451Q_NUM_ACCEL_CHANNELS	3

enum mma8451q_channel {
	MMA8451Q_CHANNEL_ACCEL_X	= 0,
	MMA8451Q_CHANNEL_ACCEL_Y,
	MMA8451Q_CHANNEL_ACCEL_Z,
};

enum mma8451q_range {
	MMA8451Q_RANGE_2G	= 0,
	MMA8451Q_RANGE_4G,
	MMA8451Q_RANGE_8G,
};

struct mma8451q_config {
	char *i2c_name;
	u8_t i2c_address;
	u8_t whoami;
	enum mma8451q_range range;
};

struct mma8451q_data {
	struct device *i2c;
	s16_t x;
	s16_t y;
	s16_t z;
};

#define SYS_LOG_DOMAIN "MMA8451Q"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#endif /* _SENSOR_MMA8451Q_ */
