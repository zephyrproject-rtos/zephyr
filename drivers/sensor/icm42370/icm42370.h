/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42370_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42370_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

typedef void (*motion_fetch_t)(const struct device *dev);
int icm42370_motion_fetch(const struct device *dev);
int icm42370_turn_on_sensor(const struct device *dev);


struct icm42370_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;
	uint16_t accel_hz;
	uint16_t accel_fs;
	int16_t temp;
	bool motion_en;
	bool sensor_started;

#ifdef CONFIG_ICM42370_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM42370_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42370_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM42370_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct icm42370_config {
#ifdef CONFIG_I2C
	struct i2c_dt_spec bus;
#elif CONFIG_SPI
	struct spi_dt_spec bus;
#endif
	struct gpio_dt_spec gpio_int;

};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42370_H_ */
