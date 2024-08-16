/* Bosch BMP180 pressure sensor
 *
 * Copyright (c) 2024 Chris Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.mouser.hk/datasheet/2/783/BST-BMP180-DS000-1509579.pdf
 */
#ifndef BMP180_H
#define BMP180_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT bosch_bmp180

#define BMP180_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union bmp180_bus {
	struct i2c_dt_spec i2c;
};

typedef int (*bmp180_bus_check_fn)(const union bmp180_bus *bus);
typedef int (*bmp180_reg_read_fn)(const union bmp180_bus *bus,
				  uint8_t start, uint8_t *buf, int size);
typedef int (*bmp180_reg_write_fn)(const union bmp180_bus *bus,
				   uint8_t reg, uint8_t val);

struct bmp180_bus_io {
	bmp180_bus_check_fn check;
	bmp180_reg_read_fn read;
	bmp180_reg_write_fn write;
};

extern const struct bmp180_bus_io bmp180_bus_io_i2c;

/* Registers */
#define BMP180_REG_CHIPID       0xD0
#define BMP180_REG_CMD          0xE0
#define BMP180_REG_MEAS_CTRL    0xF4
#define BMP180_REG_MSB          0xF6
#define BMP180_REG_LSB          0xF7
#define BMP180_REG_XLSB         0xF8
#define BMP180_REG_CALIB0       0xAA
#define BMP180_REG_CALIB21      0xBF

/* BMP180_REG_CHIPID */
#define BMP180_CHIP_ID 0x55

/* BMP180_REG_STATUS */
#define BMP180_STATUS_CMD_RDY    BIT(5)

/* BMP180_REG_CMD */
#define BMP180_CMD_SOFT_RESET 0xB6
#define BMP180_CMD_GET_TEMPERATURE 0x2E
#define BMP180_CMD_GET_OSS0_PRESS  0x34
#define BMP180_CMD_GET_OSS1_PRESS  0x74 // 0x34 | OSR<<6
#define BMP180_CMD_GET_OSS2_PRESS  0xB4 // ..
#define BMP180_CMD_GET_OSS3_PRESS  0xF4 // ..

#define BMP180_CMD_GET_OSS0_DELAY  3000
#define BMP180_CMD_GET_OSS1_DELAY  6000
#define BMP180_CMD_GET_OSS2_DELAY  12000
#define BMP180_CMD_GET_OSS3_DELAY  24000

#define BMP180_ULTRALOWPOWER 0x00  // oversampling 1x
#define BMP180_STANDARD      0x01  // oversampling 2x
#define BMP180_HIGHRES       0x02  // oversampling 4x
#define BMP180_ULTRAHIGH     0x03  // oversampling 8x

#define BSWAP_s16(x) ((int16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define BSWAP_u16(x) BSWAP_16(x)

/* Calibration Registers structure */
struct bmp180_cal_data {
	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;
} __packed;

struct bmp180_sample {
	int32_t raw_press;
	int32_t raw_temp;
	int32_t comp_temp;
};

struct bmp180_config {
	union bmp180_bus bus;
	const struct bmp180_bus_io *bus_io;
};

struct bmp180_data {
	uint8_t osr_pressure;
	struct bmp180_cal_data cal;
	struct bmp180_sample sample;
};

int bmp180_reg_field_update(const struct device *dev,
			    uint8_t reg,
			    uint8_t mask,
			    uint8_t val);

#endif /* BMP180_H */
