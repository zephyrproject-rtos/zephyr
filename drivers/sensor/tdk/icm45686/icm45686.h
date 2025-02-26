/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

//#include "imu/inv_imu_driver.h"
//#ifdef CONFIG_TDK_APEX
//#include "imu/inv_imu_apex.h"
//#endif

typedef void (*motion_fetch_t)(const struct device *dev);
int icm45686_motion_fetch(const struct device *dev);
int icm45686_turn_on_sensor(const struct device *dev);

struct icm45686_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t accel_sensitivity_shift;
	uint16_t gyro_sensitivity_x10;
	uint16_t accel_hz;
	uint16_t accel_fs;
	uint16_t gyro_hz;
	uint16_t gyro_fs;
	int16_t temp;
	bool motion_en;
	bool sensor_started;

#ifdef CONFIG_ICM45686_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM45686_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM45686_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM45686_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct icm45686_config {
#ifdef CONFIG_I2C
	struct i2c_dt_spec bus;
#elif CONFIG_SPI
	struct spi_dt_spec bus;
#endif
	struct gpio_dt_spec gpio_int;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_H_ */
