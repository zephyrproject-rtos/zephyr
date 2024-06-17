/*
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42670_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42670_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define ICM42670_BUS_SPI DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm42670, spi)
#define ICM42670_BUS_I2C DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm42670, i2c)

union icm42670_bus {
#if ICM42670_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if ICM42670_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*icm42670_bus_check_fn)(const union icm42670_bus *bus);
typedef int (*icm42670_reg_read_fn)(const union icm42670_bus *bus,
				  uint16_t reg, uint8_t *data, size_t size);
typedef int (*icm42670_reg_write_fn)(const union icm42670_bus *bus,
				   uint16_t reg, uint8_t data);

typedef int (*icm42670_reg_update_fn)(const union icm42670_bus *bus,
				   uint16_t reg, uint8_t mask, uint8_t data);

struct icm42670_bus_io {
	icm42670_bus_check_fn check;
	icm42670_reg_read_fn read;
	icm42670_reg_write_fn write;
	icm42670_reg_update_fn update;
};

#if ICM42670_BUS_SPI
extern const struct icm42670_bus_io icm42670_bus_io_spi;
#endif

#if ICM42670_BUS_I2C
extern const struct icm42670_bus_io icm42670_bus_io_i2c;
#endif

struct icm42670_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;
	uint16_t accel_hz;
	uint16_t accel_fs;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;
	uint16_t gyro_hz;
	uint16_t gyro_fs;
	int16_t temp;
#ifdef CONFIG_ICM42670_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM42670_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42670_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM42670_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct icm42670_config {
	union icm42670_bus bus;
	const struct icm42670_bus_io *bus_io;
	struct gpio_dt_spec gpio_int;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42670_H_ */
