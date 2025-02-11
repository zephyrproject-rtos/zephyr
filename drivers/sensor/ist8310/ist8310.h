/* ist8310.h - header file for IST8310 Geomagnetic sensor driver */

/*
 * Copyright (c) 2023 NXP Semiconductors
 * Copyright (c) 2023 Cognipilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IST8310_IST8310_H_
#define ZEPHYR_DRIVERS_SENSOR_IST8310_IST8310_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

#define DT_DRV_COMPAT isentek_ist8310

#define IST8310_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union ist8310_bus {
	struct i2c_dt_spec i2c;
};

typedef int (*ist8310_bus_check_fn)(const union ist8310_bus *bus);
typedef int (*ist8310_reg_read_fn)(const union ist8310_bus *bus,
				  uint8_t start, uint8_t *buf, int size);
typedef int (*ist8310_reg_write_fn)(const union ist8310_bus *bus,
				   uint8_t reg, uint8_t val);

struct ist8310_bus_io {
	ist8310_bus_check_fn check;
	ist8310_reg_read_fn read;
	ist8310_reg_write_fn write;
};

extern const struct ist8310_bus_io ist8310_bus_io_i2c;

#define IST8310_WHO_AM_I           0x00
#define IST8310_WHO_AM_I_VALUE     0x10

#define IST8310_STATUS_REGISTER1   0x02
#define STAT1_DRDY                 0x01
#define STAT1_DRO                  0x02

#define IST8310_OUTPUT_VALUE_X_L   0x03
#define IST8310_OUTPUT_VALUE_X_H   0x04
#define IST8310_OUTPUT_VALUE_Y_L   0x05
#define IST8310_OUTPUT_VALUE_Y_H   0x06
#define IST8310_OUTPUT_VALUE_Z_L   0x07
#define IST8310_OUTPUT_VALUE_Z_H   0x08

#define IST8310_CONTROL_REGISTER1  0x0A
#define CTRL1_MODE_SINGLE          0x1

#define IST8310_CONTROL_REGISTER2  0x0B
#define CTRL2_SRST                 0x01

#define IST8310_OUTPUT_VALUE_T_L   0x1C
#define IST8310_OUTPUT_VALUE_T_H   0x1D

#define IST8310_CONTROL_REGISTER3  0x0d
#define Z_16BIT			   0x40
#define Y_16BIT			   0x20
#define X_16BIT			   0x10

#define IST8310_AVG_REGISTER       0x41
#define Y_16TIMES_SET		   0x20
#define Y_16TIMES_CLEAR		   0x18
#define XZ_16TIMES_SET		   0x04
#define XZ_16TIMES_CLEAR	   0x03

#define IST8310_PDCNTL_REGISTER    0x42
#define PULSE_NORMAL		   0xC0


struct ist8310_config {
	union ist8310_bus bus;
	const struct ist8310_bus_io *bus_io;
};

struct ist8310_data {
	struct k_sem sem;
	int16_t sample_x, sample_y, sample_z;
};

int ist8310_reg_update_byte(const struct device *dev, uint8_t reg,
			   uint8_t mask, uint8_t value);

#endif /* __SENSOR_IST8310_H__ */
