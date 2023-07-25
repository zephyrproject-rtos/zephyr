/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "icm42605_reg.h"

typedef void (*tap_fetch_t)(const struct device *dev);
int icm42605_tap_fetch(const struct device *dev);

struct icm42605_data {
	uint8_t fifo_data[HARDWARE_FIFO_SIZE];

	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;
	uint16_t accel_hz;
	uint16_t accel_sf;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;
	uint16_t gyro_hz;
	uint16_t gyro_sf;

	bool accel_en;
	bool gyro_en;
	bool tap_en;

	bool sensor_started;

	const struct device *dev;
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	const struct sensor_trigger *tap_trigger;
	sensor_trigger_handler_t tap_handler;

	const struct sensor_trigger *double_tap_trigger;
	sensor_trigger_handler_t double_tap_handler;

#ifdef CONFIG_ICM42605_TRIGGER
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42605_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
};

struct icm42605_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec gpio_int;
	uint16_t accel_hz;
	uint16_t gyro_hz;
	uint16_t accel_fs;
	uint16_t gyro_fs;
};

int icm42605_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int icm42605_init_interrupt(const struct device *dev);

#endif /* __SENSOR_ICM42605__ */
