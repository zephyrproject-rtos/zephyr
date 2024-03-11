/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU6050_MPU6050_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU6050_MPU6050_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define MPU6050_REG_CHIP_ID		0x75
#define MPU6050_CHIP_ID			0x68
#define MPU9250_CHIP_ID			0x71
#define MPU6880_CHIP_ID			0x19

#define MPU6050_REG_GYRO_CFG		0x1B
#define MPU6050_GYRO_FS_SHIFT		3

#define MPU6050_REG_ACCEL_CFG		0x1C
#define MPU6050_ACCEL_FS_SHIFT		3

#define MPU6050_REG_INT_EN		0x38
#define MPU6050_DRDY_EN			BIT(0)

#define MPU6050_REG_DATA_START		0x3B

#define MPU6050_REG_PWR_MGMT1		0x6B
#define MPU6050_SLEEP_EN		BIT(6)

/* measured in degrees/sec x10 to avoid floating point */
static const uint16_t mpu6050_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct mpu6050_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

#ifdef CONFIG_MPU6050_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_MPU6050_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MPU6050_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_MPU6050_TRIGGER */
};

struct mpu6050_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_MPU6050_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif /* CONFIG_MPU6050_TRIGGER */
};

#ifdef CONFIG_MPU6050_TRIGGER
int mpu6050_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu6050_init_interrupt(const struct device *dev);
#endif

#endif /* __SENSOR_MPU6050__ */
