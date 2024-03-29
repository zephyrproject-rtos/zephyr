/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_
#define ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif
#include <zephyr/sys/util.h>

/* ADXL345 communication commands */
#define ADXL345_WRITE_CMD      0x00
#define ADXL345_READ_CMD       0x80
#define ADXL345_MULTIBYTE_FLAG 0x40

/* Registers */
/* ADXL345 DEVICE_ID register */
#define ADXL345_DEVICE_ID_REG   0x00
#define ADXL345_DEVICE_ID_VALUE 0b11100101

/* ADXL345 THRESH_TAP register */
#define ADXL345_THRESH_TAP_REG 0x1D

/* ADXL345 DUR register */
#define ADXL345_DUR_REG 0x21

/* ADXL345 THRESH_ACT register */
#define ADXL345_THRESH_ACT_REG 0x24

/* ADXL345 THRESH_INACT register */
#define ADXL345_THRESH_INACT_REG 0x25

/* ADXL345 TIME_INACT register */
#define ADXL345_TIME_INACT_REG 0x26

/* ADXL345 ACT_INACT_CTL register */
#define ADXL345_ACT_INACT_CTL_REG                0x27
#define ADXL345_ACT_INACT_CTL_ACT_DECOUPLE_MSK   BIT(7)
#define ADXL345_ACT_INACT_CTL_ACT_AXIS_MSK       GENMASK(6, 4)
#define ADXL345_ACT_INACT_CTL_INACT_DECOUPLE_MSK BIT(3)
#define ADXL345_ACT_INACT_CTL_INACT_AXIS_MSK     GENMASK(2, 0)

/* ADXL345 THRESH_FF register */
#define ADXL345_THRESH_FF_REG 0x28
/* ADXL345 TIME_FF register */
#define ADXL345_TIME_FF_REG   0x29

/* ADXL345 TAP_AXES register */
#define ADXL345_TAP_AXES_REG          0x2A
#define ADXL345_TAP_AXES_TAP_AXIS_MSK GENMASK(2, 0)

/* ADXL345 BW_RATE register */
#define ADXL345_BW_RATE_REG           0x2C
#define ADXL345_BW_RATE_RATE_MSK      GENMASK(3, 0)
#define ADXL345_BW_RATE_LOW_POWER_MSK BIT(4)

/* ADXL345 POWER_CTL register */
#define ADXL345_POWER_CTL_REG             0x2D
#define ADXL345_POWER_CTL_REG_MEASURE_BIT BIT(3)
#define ADXL345_POWER_CTL_LINK_BIT        BIT(5)

/* ADXL345 INT_ENABLE register */
#define ADXL345_INT_ENABLE_REG            0x2E
#define ADXL345_INT_ENABLE_WATERMARK_BIT  BIT(1)
#define ADXL345_INT_ENABLE_FREE_FALL_BIT  BIT(2)
#define ADXL345_INT_ENABLE_INACTIVE_BIT   BIT(3)
#define ADXL345_INT_ENABLE_ACTIVE_BIT     BIT(4)
#define ADXL345_INT_ENABLE_DOUBLE_TAP_BIT BIT(5)
#define ADXL345_INT_ENABLE_SINGLE_TAP_BIT BIT(6)
#define ADXL345_INT_ENABLE_DATA_READY_BIT BIT(7)

/* ADXL345 INT_SOURCE register */
#define ADXL345_INT_SOURCE_REG            0x30
#define ADXL345_INT_SOURCE_WATERMARK_BIT  BIT(1)
#define ADXL345_INT_SOURCE_FREE_FALL_BIT  BIT(2)
#define ADXL345_INT_SOURCE_INACTIVE_BIT   BIT(3)
#define ADXL345_INT_SOURCE_ACTIVE_BIT     BIT(4)
#define ADXL345_INT_SOURCE_DOUBLE_TAP_BIT BIT(5)
#define ADXL345_INT_SOURCE_SINGLE_TAP_BIT BIT(6)
#define ADXL345_INT_SOURCE_DATA_READY_BIT BIT(7)

/* ADXL345 DATA_FORMAT register */
#define ADXL345_DATA_FORMAT_REG       0x31
#define ADXL345_DATA_FORMAT_RANGE_MSK GENMASK(1, 0)

/* ADXL345 DATAX0, etc registers */
#define ADXL345_X_AXIS_DATA_0_REG 0x32

/* ADXL345 FIFO_CTL register */
#define ADXL345_FIFO_CTL_REG        0x38
#define ADXL345_FIFO_CTL_FIFO_MSK   GENMASK(7, 6)
#define ADXL345_FIFO_CTL_SAMPLE_MSK GENMASK(4, 0)

/* ADXL345 FIFO_STATUS register */
#define ADXL345_FIFO_STATUS_REG 0x39

/* Sensor properties definitions */
/* ADXL345 FIFO size */
#define ADXL345_MAX_FIFO_SIZE 32
#define ADXL345_COMPLEMENT    0xfc00

/* ADXL345 supported sampling frequencies */
enum adxl345_odr {
	ADXL345_ODR_6P25HZ = BIT(2) | BIT(1),
	ADXL345_ODR_12P5HZ = BIT(2) | BIT(1) | BIT(0),
	ADXL345_ODR_25HZ = BIT(3),
	ADXL345_ODR_50HZ = BIT(3) | BIT(0),
	ADXL345_ODR_100HZ = BIT(3) | BIT(1),
	ADXL345_ODR_200HZ = BIT(3) | BIT(2),
	ADXL345_ODR_400HZ = BIT(3) | BIT(2) | BIT(0),
};

/* ADXL345 supported sampling ranges */
enum adxl345_range {
	ADXL345_2G_RANGE = 0,
	ADXL345_4G_RANGE = BIT(0),
	ADXL345_8G_RANGE = BIT(1),
	ADXL345_16G_RANGE = BIT(1) | BIT(0),
};

/* ADXL345 supported fifo modes */
enum adxl345_fifo {
	ADXL345_FIFO_BYPASS = 0,
	ADXL345_FIFO_FIFO = BIT(6),
	ADXL345_FIFO_STREAM = BIT(7),
	ADXL345_FIFO_TRIGGER = GENMASK(7, 6),
};

/* ADXL345 sample */
struct adxl345_sample {
	int16_t x;
	int16_t y;
	int16_t z;
};

struct adxl345_dev_data {
	unsigned int sample_number;

	int16_t bufx[ADXL345_MAX_FIFO_SIZE];
	int16_t bufy[ADXL345_MAX_FIFO_SIZE];
	int16_t bufz[ADXL345_MAX_FIFO_SIZE];

	enum adxl345_odr odr;
	enum adxl345_range range;
	enum adxl345_fifo fifo_mode;
	uint16_t scale_factor;
};

union adxl345_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
};

typedef bool (*adxl345_bus_is_ready_fn)(const union adxl345_bus *bus);
typedef int (*adxl345_reg_access_fn)(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				     uint8_t *data, size_t length);

struct adxl345_dev_config {
	const union adxl345_bus bus;
	adxl345_bus_is_ready_fn bus_is_ready;
	adxl345_reg_access_fn reg_access;

	uint16_t frequency;
	uint8_t range;
	uint8_t fifo_mode;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_ */
