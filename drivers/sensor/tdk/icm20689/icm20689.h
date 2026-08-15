/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20689_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20689_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#define ICM20689_GYRO_DATA_SIZE  6
#define ICM20689_ACCEL_DATA_SIZE 6
#define ICM20689_TEMP_DATA_SIZE  2

#define ICM20689_DEVICE_ID 0x98

struct icm20689_sensor_config {
	uint16_t accel_fs;
	uint16_t gyro_fs;
	uint8_t accel_dlpf;
	uint8_t gyro_dlpf;
	uint8_t smplrt_div;
};

struct icm20689_axis_alignment {
	uint8_t index;
	int8_t sign;
};

struct icm20689_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec gpio_int;
	struct icm20689_axis_alignment axis_align[3];

	/* Initial sensor configuration from Devicetree. */
	struct icm20689_sensor_config initial;
};

struct icm20689_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	int16_t temp;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;

	/* Currently active configuration. */
	struct icm20689_sensor_config active;

	uint16_t accel_sensitivity_shift;
	uint16_t gyro_sensitivity_x10;
#ifdef CONFIG_ICM20689_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM20689_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM20689_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM20689_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20689_H_ */
