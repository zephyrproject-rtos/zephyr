/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_H_

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/util.h>
#include <zephyr/types.h>

#include "icm42605_reg.h"

typedef void (*tap_fetch_t)(const struct device *dev);
int icm42605_tap_fetch(const struct device *dev);

struct icm42605_data {
	const struct device *spi;

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
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	struct sensor_trigger tap_trigger;
	sensor_trigger_handler_t tap_handler;

	struct sensor_trigger double_tap_trigger;
	sensor_trigger_handler_t double_tap_handler;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42605_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;

	struct spi_cs_control spi_cs;
	struct spi_config spi_cfg;
};

struct icm42605_config {
	const char *spi_label;
	uint16_t spi_addr;
	uint32_t frequency;
	uint32_t slave;
	uint8_t int_pin;
	uint8_t int_flags;
	const char *int_label;
	const char *gpio_label;
	gpio_pin_t gpio_pin;
	gpio_dt_flags_t gpio_dt_flags;
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
