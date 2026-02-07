/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_H_
#define ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#include "bhy2.h"

/* BHI2xy work buffer size for FIFO processing */
#define BHI2XY_WORK_BUFFER_SIZE CONFIG_BHI2XY_WORK_BUFFER_SIZE

enum bhi2xy_variant {
	BHI2XY_VARIANT_BHI260AB,
	BHI2XY_VARIANT_BHI260AP,
};

union bhi2xy_bus {
#if CONFIG_BHI2XY_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_BHI2XY_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*bhi2xy_bus_check_fn)(const union bhi2xy_bus *bus);
typedef int (*bhi2xy_reg_read_fn)(const union bhi2xy_bus *bus, uint8_t reg, uint8_t *data,
				  uint16_t len);
typedef int (*bhi2xy_reg_write_fn)(const union bhi2xy_bus *bus, uint8_t reg, const uint8_t *data,
				   uint16_t len);
typedef enum bhy2_intf (*bhi2xy_get_intf_fn)(const union bhi2xy_bus *bus);

struct bhi2xy_bus_io {
	bhi2xy_bus_check_fn check;
	bhi2xy_reg_read_fn read;
	bhi2xy_reg_write_fn write;
	bhi2xy_get_intf_fn get_intf;
};

struct bhi2xy_config {
	union bhi2xy_bus bus;
	const struct bhi2xy_bus_io *bus_io;
	enum bhi2xy_variant variant;
	const struct gpio_dt_spec reset_gpio;
};

struct bhi2xy_data {
	struct bhy2_dev bhy2;
	uint8_t work_buffer[BHI2XY_WORK_BUFFER_SIZE];
	/* current ranges for accel and gyro, obtained during init used for unit conversions */
	uint16_t acc_range;
	uint16_t gyro_range;
	/* there are a lot of virtual sensors, we reserve data for some of the most probably used
	 * ones so far, but can be extended later */
	int16_t acc[3], gyro[3], mag[3];
	int16_t euler[3];      /* orientation */
	int16_t grv[4], rv[4]; /* game rotation vector (relative orientation) and rotation vector
				  (absolute orientation) */
	int32_t pres;          /* pressure */
	uint32_t step_count;
};

#if CONFIG_BHI2XY_BUS_SPI
extern const struct bhi2xy_bus_io bhi2xy_bus_io_spi;
#endif

#if CONFIG_BHI2XY_BUS_I2C
extern const struct bhi2xy_bus_io bhi2xy_bus_io_i2c;
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_H_ */
