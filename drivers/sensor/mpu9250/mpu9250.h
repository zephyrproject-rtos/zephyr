/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU9250_MPU9250_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU9250_MPU9250_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

struct mpu9250_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

#ifdef CONFIG_MPU9250_MAGN_EN
	int16_t magn_x;
	int16_t magn_scale_x;
	int16_t magn_y;
	int16_t magn_scale_y;
	int16_t magn_z;
	int16_t magn_scale_z;
	uint8_t magn_st2;
#endif

#ifdef CONFIG_MPU9250_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_MPU9250_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MPU9250_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU9250_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_MPU9250_TRIGGER */
};

struct mpu9250_config {
	const struct i2c_dt_spec i2c;
	uint8_t gyro_sr_div;
	uint8_t gyro_dlpf;
	uint8_t gyro_fs;
	uint8_t accel_fs;
	uint8_t accel_dlpf;
#ifdef CONFIG_MPU9250_TRIGGER
	const struct gpio_dt_spec int_pin;
#endif /* CONFIG_MPU9250_TRIGGER */
};

#ifdef CONFIG_MPU9250_TRIGGER
int mpu9250_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu9250_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_MPU9250_MPU9250_H_ */
