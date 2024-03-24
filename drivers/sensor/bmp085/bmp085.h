/*
 * Copyright (c) 2024, Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP085_BMP085_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP085_BMP085_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

/* registers */
#define BMP085_REG_CHIPID     0xD0
#define BMP085_REG_VERSION    0xD1
#define BMP085_REG_CAL_COEF   0xAA
#define BMP085_CMD_READ_TEMP  0x2E /* read temp command */
#define BMP085_CMD_READ_PRESS 0x34 /* read pressure command */
#define BMP085_CTRL           0xF4 /* control register */
#define BMP085_REG_MSB        0xF6 /* read adc msb register */
#define BMP085_REG_LSB        0xF7 /* read adc lsb  register */
#define BMP085_REG_XLSB       0xF8 /* read adc xlsb register */

#define BMP085_CHIP_ID 0x55

enum bmp085_oversampling {
	BMP085_MODE_1_ULTRALOWPOWER = 0,
	BMP085_MODE_2_STANDARD,
	BMP085_MODE_3_HIGHRES,
	BMP085_MODE_4_ULTRAHIGHRES,
};

struct bmp085_config {
	struct i2c_dt_spec i2c;
	enum bmp085_oversampling oversampling;
};

struct bmp085_data {
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

	int64_t raw_temp;
	int64_t raw_press;
	int32_t temp;
	int32_t press;

	uint8_t chip_id;
};

static int bmp085_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len);
static int bmp085_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *byte);
static int bmp085_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t data);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP085_BMP085_H_ */
