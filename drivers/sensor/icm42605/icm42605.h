/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <zephyr/types.h>

static const uint16_t icm42605_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct icm42605_data {
	struct device *spi;

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

	struct device *dev;
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	struct sensor_trigger tap_trigger;
	sensor_trigger_handler_t tap_handler;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42605_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
};

struct icm42605_config {
	const char *spi_label;
	uint16_t spi_addr;
	uint8_t int_pin;
	uint8_t int_flags;
	const char *int_label;
};

int icm42605_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int icm42605_init_interrupt(struct device *dev);

#endif /* __SENSOR_ICM42605__ */
