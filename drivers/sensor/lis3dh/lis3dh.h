/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS3DH_LIS3DH_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS3DH_LIS3DH_H_

#include <device.h>
#include <misc/util.h>
#include <zephyr/types.h>
#include <gpio.h>

#define LIS3DH_I2C_ADDRESS		CONFIG_LIS3DH_I2C_ADDR

#define LIS3DH_AUTOINCREMENT_ADDR	BIT(7)

#define LIS3DH_REG_CTRL1		0x20
#define LIS3DH_ACCEL_X_EN_BIT		BIT(0)
#define LIS3DH_ACCEL_Y_EN_BIT		BIT(1)
#define LIS3DH_ACCEL_Z_EN_BIT		BIT(2)
#define LIS3DH_ACCEL_EN_BITS (LIS3DH_ACCEL_X_EN_BIT | \
		LIS3DH_ACCEL_Y_EN_BIT | LIS3DH_ACCEL_Z_EN_BIT)

#if defined(CONFIG_LIS3DH_POWER_MODE_LOW)
	#define LIS3DH_LP_EN_BIT	BIT(3)
#elif defined(CONFIG_LIS3DH_POWER_MODE_NORMAL)
	#define LIS3DH_LP_EN_BIT	0
#endif

#if defined(CONFIG_LIS3DH_ODR_1)
	#define LIS3DH_ODR_IDX		1
#elif defined(CONFIG_LIS3DH_ODR_2)
	#define LIS3DH_ODR_IDX		2
#elif defined(CONFIG_LIS3DH_ODR_3)
	#define LIS3DH_ODR_IDX		3
#elif defined(CONFIG_LIS3DH_ODR_4)
	#define LIS3DH_ODR_IDX		4
#elif defined(CONFIG_LIS3DH_ODR_5)
	#define LIS3DH_ODR_IDX		5
#elif defined(CONFIG_LIS3DH_ODR_6)
	#define LIS3DH_ODR_IDX		6
#elif defined(CONFIG_LIS3DH_ODR_7)
	#define LIS3DH_ODR_IDX		7
#elif defined(CONFIG_LIS3DH_ODR_8)
	#define LIS3DH_ODR_IDX		8
#elif defined(CONFIG_LIS3DH_ODR_9_NORMAL) || defined(CONFIG_LIS3DH_ODR_9_LOW)
	#define LIS3DH_ODR_IDX		9
#endif

#define LIS3DH_ODR_SHIFT		4
#define LIS3DH_ODR_BITS			(LIS3DH_ODR_IDX << LIS3DH_ODR_SHIFT)

#define LIS3DH_REG_CTRL3		0x22
#define LIS3DH_EN_DRDY1_INT1		BIT(4)

#define LIS3DH_REG_CTRL4		0x23
#define LIS3DH_FS_SHIFT			4
#define LIS3DH_FS_MASK			(BIT_MASK(2) << LIS3DH_FS_SHIFT)

#if defined(CONFIG_LIS3DH_ACCEL_RANGE_2G)
	#define LIS3DH_FS_IDX		0
#elif defined(CONFIG_LIS3DH_ACCEL_RANGE_4G)
	#define LIS3DH_FS_IDX		1
#elif defined(CONFIG_LIS3DH_ACCEL_RANGE_8G)
	#define LIS3DH_FS_IDX		2
#elif defined(CONFIG_LIS3DH_ACCEL_RANGE_16G)
	#define LIS3DH_FS_IDX		3
#endif

#define LIS3DH_FS_BITS			(LIS3DH_FS_IDX << LIS3DH_FS_SHIFT)
#define LIS3DH_ACCEL_SCALE		(SENSOR_G * (4 << LIS3DH_FS_IDX))

#define LIS3DH_REG_ACCEL_X_LSB		0x28
#define LIS3DH_REG_ACCEL_Y_LSB		0x2A
#define LIS3DH_REG_ACCEL_Z_LSB		0x2C
#define LIS3DH_REG_ACCEL_X_MSB		0x29
#define LIS3DH_REG_ACCEL_Y_MSB		0x2B
#define LIS3DH_REG_ACCEL_Z_MSB		0x2D

struct lis3dh_data {
	struct device *i2c;
	s16_t x_sample;
	s16_t y_sample;
	s16_t z_sample;

#ifdef CONFIG_LIS3DH_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_LIS3DH_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS3DH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS3DH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_LIS3DH_TRIGGER */
};

#ifdef CONFIG_LIS3DH_TRIGGER
int lis3dh_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int lis3dh_sample_fetch(struct device *dev, enum sensor_channel chan);

int lis3dh_init_interrupt(struct device *dev);
#endif

#define SYS_LOG_DOMAIN "LIS3DH"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* __SENSOR_LIS3DH__ */
