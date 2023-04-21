/* ST Microelectronics LSM6DSR 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsr.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lsm6dsr_reg.h"
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#define LSM6DSR_EN_BIT			0x01
#define LSM6DSR_DIS_BIT			0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define SENSI_GRAIN_XL			(61LL)

/* Gyro sensor sensitivity grain is 4375 udps/LSB */
#define SENSI_GRAIN_G			(4375LL)
#define SENSOR_PI_DOUBLE		(SENSOR_PI / 1000000.0)
#define SENSOR_DEG2RAD_DOUBLE		(SENSOR_PI_DOUBLE / 180)
#define SENSOR_G_DOUBLE			(SENSOR_G / 1000000.0)

struct lsm6dsr_config {
	stmdev_ctx_t ctx;
	union {
		const struct i2c_dt_spec i2c;
		const struct spi_dt_spec spi;
	} stmemsc_cfg;
	uint8_t accel_pm;
	uint8_t accel_odr;
	uint8_t accel_range;
	uint8_t gyro_pm;
	uint8_t gyro_odr;
	uint8_t gyro_range;
	bool drdy_pulsed;
#ifdef CONFIG_LSM6DSR_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	uint8_t int_pin;
	bool trig_enabled;
#endif
};

struct lsm6dsr_data {
	const struct device *dev;
	float accel_freq;
	uint32_t acc_gain;
	int16_t acc[3];
	uint8_t accel_pm;
	uint8_t accel_fs;
	float gyro_freq;
	uint32_t gyro_gain;
	int16_t gyro[3];
	uint8_t gyro_fs;
	int16_t temp_sample;

#ifdef CONFIG_LSM6DSR_TRIGGER
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	const struct sensor_trigger *trig_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;
	const struct sensor_trigger *trig_drdy_temp;

#if defined(CONFIG_LSM6DSR_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LSM6DSR_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LSM6DSR_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LSM6DSR_TRIGGER */
};

int lsm6dsr_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lsm6dsr_init_interrupt(const struct device *dev);


#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_ */
