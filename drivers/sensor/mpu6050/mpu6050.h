/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU6050_MPU6050_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU6050_MPU6050_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <zephyr/types.h>

#define MPU6050_REG_CHIP_ID		0x75
#ifdef CONFIG_MPU6050_MPU9250_MODE
#define MPU6050_REG_VALUE_CHIP_ID	0x71
#else
#define MPU6050_REG_VALUE_CHIP_ID	0x68
#endif

#define MPU6050_REG_GYRO_CFG		0x1B
#define MPU6050_GYRO_FS_SHIFT		3

#define MPU6050_REG_ACCEL_CFG		0x1C
#define MPU6050_ACCEL_FS_SHIFT		3

#define MPU6050_REG_INT_EN		0x38
#define MPU6050_DRDY_EN			BIT(0)

#define MPU6050_REG_DATA_START		0x3B

#define MPU6050_REG_PWR_MGMT1			0x6B
#define MPU6050_REG_VALUE_PWR_MGMT1_RESET	0x80
#define MPU6050_REG_VALUE_PWR_MGMT1_CLOCK_PLL	0x01

#ifdef CONFIG_MPU6050_MPU9250_WITH_AK
#define MPU9250_AK89XX_REG_CNTL1			0x0A
#define MPU9250_AK89XX_REG_VALUE_CNTL1_POWERDOWN	0x00
#define MPU9250_AK89XX_REG_VALUE_CNTL1_FUSE_ROM	0x0F
#define MPU9250_AK89XX_REG_VALUE_CNTL1_16BIT_100HZ	0x16
#define MPU9250_AK89XX_REG_DATA			0x03
#define MPU9250_AK89XX_REG_ID				0x00
#define MPU9250_AK89XX_REG_VALUE_ID			0x48
#define MPU9250_AK89XX_REG_CNTL2			0x0B
#define MPU9250_AK89XX_REG_VALUE_CNTL2_RESET		0x01
#define MPU9250_AK89XX_REG_ADJ_DATA			0x10
#define MPU9250_REG_I2C_MST_CTRL			0x24
#define MPU9250_REG_VALUE_I2C_MST_CTRL_WAIT_MAG_400KHZ	0x4D
#define MPU9250_REG_I2C_SLV0_ADDR			0x25
#define MPU9250_REG_VALUE_I2C_SLV0_ADDR_AK89XX		0x0C
#define MPU9250_REG_I2C_SLV0_REG			0x26
#define MPU9250_REG_I2C_SLV0_CTRL			0x27
#define MPU9250_REG_I2C_SLV0_DATA0			0x63
#define MPU9250_REG_VALUE_I2C_SLV0_CTRL		0x80
#define MPU9250_REG_USER_CTRL				0x6A
#define MPU9250_REG_VALUE_USER_CTRL_I2C_MASTERMODE	0x20
#define MPU9250_REG_EXT_DATA00				0x49
#endif /* CONFIG_MPU6050_MPU9250_WITH_AK */

/* measured in degrees/sec x10 to avoid floating point */
static const u16_t mpu6050_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct mpu6050_data {
	struct device *i2c;

	s16_t accel_x;
	s16_t accel_y;
	s16_t accel_z;
	u16_t accel_sensitivity_shift;

	s16_t temp;

	s16_t gyro_x;
	s16_t gyro_y;
	s16_t gyro_z;
	u16_t gyro_sensitivity_x10;
#ifdef CONFIG_MPU6050_MPU9250_WITH_AK
	s16_t magn_x;
	s16_t magn_y;
	s16_t magn_z;
	s16_t magn_scale_x;
	s16_t magn_scale_y;
	s16_t magn_scale_z;
#endif /* CONFIG_MPU6050_MPU9250_WITH_AK */

#ifdef CONFIG_MPU6050_TRIGGER
	struct device *dev;
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_MPU6050_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_MPU6050_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_MPU6050_TRIGGER */
};

struct mpu6050_config {
	const char *i2c_label;
	u16_t i2c_addr;
#ifdef CONFIG_MPU6050_TRIGGER
	u8_t int_pin;
	u8_t int_flags;
	const char *int_label;
#endif /* CONFIG_MPU6050_TRIGGER */
};

#ifdef CONFIG_MPU6050_TRIGGER
int mpu6050_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu6050_init_interrupt(struct device *dev);
#endif /* CONFIG_MPU6050_TRIGGER */

#ifdef CONFIG_MPU6050_MPU9250_WITH_AK
int mpu6050_ak89xx_init(struct device *dev);
void mpu6050_ak89xx_convert_magn(struct sensor_value *val, s16_t raw_val,
				  s16_t scale);
#endif /* CONFIG_MPU6050_MPU9250_WITH_AK */

#endif /* __SENSOR_MPU6050__ */
