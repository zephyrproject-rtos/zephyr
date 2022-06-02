/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT 	bosch_bmp180
#define BMP180_BUS_I2C 	DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union bmp180_bus {
#if BMP180_BUS_I2C
	struct i2c_dt_spec i2c;
#else
	#error Proper bus for BMP180 sensor not found
#endif
};

typedef int (*bmp180_bus_check_fn)(const union bmp180_bus *bus);
typedef int (*bmp180_reg_read_fn)(const union bmp180_bus *bus, uint8_t start, uint8_t *buf, int size);
typedef int (*bmp180_reg_write_fn)(const union bmp180_bus *bus, uint8_t reg, uint8_t val);

struct bmp180_bus_io {
	bmp180_bus_check_fn check;
	bmp180_reg_read_fn read;
	bmp180_reg_write_fn write;
};

#if BMP180_BUS_I2C
extern const struct bmp180_bus_io bmp180_bus_io_i2c;
#endif

#define SENSOR_CHAN_BMP180_ALTITUDE	sensor_channel::SENSOR_CHAN_PRIV_START + 1

#define BMP180_REG_ID					0xD0
#define BMP180_REG_COMP_START			0xAA
#define BMP180_REG_RESET				0xE0
#define BMP180_REG_MEAS_CTRL			0xF4
#define BMP180_REG_OUT_MSB				0xF6
#define BMP180_REG_OUT_LSB				0xF7
#define BMP180_REG_OUT_XLSB				0xF8

#define BMP_START_CONVERSION			(1UL << 5)
#define BMP_SELECT_TEMPERATURE			0x0E
#define BMP_SELECT_PRESSURE				0x14
#define BMP180_CMD_SOFT_RESET			0xB6

#define BMP180_CHIP_ID					0x55

#define BMP180_OVERSAMPLING_REGVALUE(OVERSAMPLING)	(OVERSAMPLING << 6)

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_ */
