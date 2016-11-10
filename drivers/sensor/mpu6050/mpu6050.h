/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SENSOR_MPU6050_H__
#define __SENSOR_MPU6050_H__

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <stdint.h>

#define SYS_LOG_DOMAIN "MPU6050"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>

#define MPU6050_REG_CHIP_ID		0x75
#define MPU6050_CHIP_ID			0x68

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
	struct device *i2c;

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
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_MPU6050_TRIGGER_OWN_THREAD)
	char __stack thread_stack[CONFIG_MPU6050_THREAD_STACK_SIZE];
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_MPU6050_TRIGGER */
};

#ifdef CONFIG_MPU6050_TRIGGER
int mpu6050_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu6050_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_MPU6050__ */
