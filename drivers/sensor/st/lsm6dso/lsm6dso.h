/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lsm6dso_reg.h"

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso32, spi)
#include <zephyr/drivers/spi.h>
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso32, i2c)
#include <zephyr/drivers/i2c.h>
#endif

#define LSM6DSO_EN_BIT					0x01
#define LSM6DSO_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL				(61LL)

/* Gyro sensor sensitivity grain is 4.375 udps/LSB */
#define GAIN_UNIT_G				(4375LL)

struct lsm6dso_config {
	stmdev_ctx_t ctx;
	union {
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso32, i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lsm6dso32, spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t accel_pm;
	uint8_t accel_odr;
#define ACCEL_RANGE_DOUBLE	BIT(7)
#define ACCEL_RANGE_MASK	BIT_MASK(6)
	uint8_t accel_range;
	uint8_t gyro_pm;
	uint8_t gyro_odr;
	uint8_t gyro_range;
	uint8_t drdy_pulsed;
#ifdef CONFIG_LSM6DSO_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	uint8_t int_pin;
	bool trig_enabled;
#endif /* CONFIG_LSM6DSO_TRIGGER */
};

#define LSM6DSO_SHUB_MAX_NUM_TARGETS			3

struct lsm6dso_data {
	const struct device *dev;
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	int16_t temp_sample;
#endif
#if defined(CONFIG_LSM6DSO_SENSORHUB)
	uint8_t ext_data[LSM6DSO_SHUB_MAX_NUM_TARGETS][6];
	uint16_t magn_gain;

	struct hts221_data {
		int16_t x0;
		int16_t x1;
		int16_t y0;
		int16_t y1;
	} hts221;
	bool shub_inited;
	uint8_t num_ext_dev;
	uint8_t shub_ext[LSM6DSO_SHUB_MAX_NUM_TARGETS];
#endif /* CONFIG_LSM6DSO_SENSORHUB */

	uint16_t accel_freq;
	uint8_t accel_fs;
	uint16_t gyro_freq;
	uint8_t gyro_fs;

#ifdef CONFIG_LSM6DSO_TRIGGER
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	const struct sensor_trigger *trig_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;
	const struct sensor_trigger *trig_drdy_temp;

#if defined(CONFIG_LSM6DSO_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LSM6DSO_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LSM6DSO_TRIGGER */
};

#if defined(CONFIG_LSM6DSO_SENSORHUB)
int lsm6dso_shub_init(const struct device *dev);
int lsm6dso_shub_fetch_external_devs(const struct device *dev);
int lsm6dso_shub_get_idx(const struct device *dev, enum sensor_channel type);
int lsm6dso_shub_config(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val);
#endif /* CONFIG_LSM6DSO_SENSORHUB */

#ifdef CONFIG_LSM6DSO_TRIGGER
int lsm6dso_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lsm6dso_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSO_LSM6DSO_H_ */
