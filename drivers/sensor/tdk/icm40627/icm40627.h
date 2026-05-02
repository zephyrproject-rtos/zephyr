/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM40627_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM40627_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define ICM40627_BUS_SPI DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm40627, spi)
#define ICM40627_BUS_I2C DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm40627, i2c)

union icm40627_bus {
#if ICM40627_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*icm40627_bus_check_fn)(const union icm40627_bus *bus);
typedef int (*icm40627_reg_read_fn)(const union icm40627_bus *bus, uint16_t reg, uint8_t *data,
				    size_t size);
typedef int (*icm40627_reg_write_fn)(const union icm40627_bus *bus, uint16_t reg, uint8_t data);

typedef int (*icm40627_reg_update_fn)(const union icm40627_bus *bus, uint16_t reg, uint8_t mask,
				      uint8_t data);

struct icm40627_bus_io {
	icm40627_bus_check_fn check;
	icm40627_reg_read_fn read;
	icm40627_reg_write_fn write;
	icm40627_reg_update_fn update;
};

#if ICM40627_BUS_I2C
extern const struct icm40627_bus_io icm40627_bus_io_i2c;
#endif

struct icm40627_data {
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
#ifdef CONFIG_ICM40627_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM40627_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM40627_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM40627_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct icm40627_config {
	union icm40627_bus bus;
	const struct icm40627_bus_io *bus_io;
	struct gpio_dt_spec gpio_int;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM40627_H_ */
