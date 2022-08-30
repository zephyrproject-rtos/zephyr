/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_
#define ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_

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
#define ADXL345_WRITE_CMD          0x00
#define ADXL345_READ_CMD           0x80
#define ADXL345_MULTIBYTE_FLAG     0x40

/* Registers */
#define ADXL345_DEVICE_ID_REG      0x00
#define ADXL345_RATE_REG           0x2c
#define ADXL345_POWER_CTL_REG      0x2d
#define ADXL345_DATA_FORMAT_REG    0x31
#define ADXL345_X_AXIS_DATA_0_REG  0x32
#define ADXL345_FIFO_CTL_REG       0x38
#define ADXL345_FIFO_STATUS_REG    0x39

#define ADXL345_PART_ID            0xe5

#define ADXL345_RANGE_2G           0x0
#define ADXL345_RANGE_4G           0x1
#define ADXL345_RANGE_8G           0x2
#define ADXL345_RANGE_16G          0x3
#define ADXL345_RATE_25HZ          0x8
#define ADXL345_ENABLE_MEASURE_BIT (1 << 3)
#define ADXL345_FIFO_STREAM_MODE   (1 << 7)
#define ADXL345_FIFO_COUNT_MASK    0x3f
#define ADXL345_COMPLEMENT         0xfc00

#define ADXL345_MAX_FIFO_SIZE      32

struct adxl345_dev_data {
	unsigned int sample_number;

	int16_t bufx[ADXL345_MAX_FIFO_SIZE];
	int16_t bufy[ADXL345_MAX_FIFO_SIZE];
	int16_t bufz[ADXL345_MAX_FIFO_SIZE];
};

struct adxl345_sample {
	int16_t x;
	int16_t y;
	int16_t z;
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
typedef int (*adxl345_reg_access_fn)(const struct device *dev, uint8_t cmd,
				     uint8_t reg_addr, uint8_t *data, size_t length);

struct adxl345_dev_config {
	const union adxl345_bus bus;
	adxl345_bus_is_ready_fn bus_is_ready;
	adxl345_reg_access_fn reg_access;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_ */
