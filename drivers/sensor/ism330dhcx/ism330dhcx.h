/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_
#define ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "ism330dhcx_reg.h"

#define ISM330DHCX_EN_BIT					0x01
#define ISM330DHCX_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL				(61LL)

/* Gyro sensor sensitivity grain is 4.375 udps/LSB */
#define GAIN_UNIT_G				(4375LL)

#define SENSOR_PI_DOUBLE			(SENSOR_PI / 1000000.0)
#define SENSOR_DEG2RAD_DOUBLE			(SENSOR_PI_DOUBLE / 180)
#define SENSOR_G_DOUBLE				(SENSOR_G / 1000000.0)

struct ism330dhcx_config {
	int (*bus_init)(const struct device *dev);
	uint8_t accel_odr;
	uint16_t gyro_odr;
	uint8_t accel_range;
	uint16_t gyro_range;
#ifdef CONFIG_ISM330DHCX_TRIGGER
	uint8_t int_pin;
	struct gpio_dt_spec drdy_gpio;
#endif /* CONFIG_ISM330DHCX_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
};

union samples {
	uint8_t raw[6];
	struct {
		int16_t axis[3];
	};
} __aligned(2);

#define ISM330DHCX_SHUB_MAX_NUM_SLVS			2

struct ism330dhcx_data {
	const struct device *dev;
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	int temp_sample;
#endif
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
	uint8_t ext_data[2][6];
	uint16_t magn_gain;

	struct hts221_data {
		int16_t x0;
		int16_t x1;
		int16_t y0;
		int16_t y1;
	} hts221;
#endif /* CONFIG_ISM330DHCX_SENSORHUB */

	stmdev_ctx_t *ctx;

	#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
	#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
	#endif

	uint16_t accel_freq;
	uint8_t accel_fs;
	uint16_t gyro_freq;
	uint8_t gyro_fs;

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_drdy_gyr;
	const struct sensor_trigger *trig_drdy_gyr;
	sensor_trigger_handler_t handler_drdy_temp;
	const struct sensor_trigger *trig_drdy_temp;

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ISM330DHCX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ISM330DHCX_TRIGGER */
};

int ism330dhcx_spi_init(const struct device *dev);
int ism330dhcx_i2c_init(const struct device *dev);
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
int ism330dhcx_shub_init(const struct device *dev);
int ism330dhcx_shub_fetch_external_devs(const struct device *dev);
int ism330dhcx_shub_get_idx(enum sensor_channel type);
int ism330dhcx_shub_config(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val);
#endif /* CONFIG_ISM330DHCX_SENSORHUB */

#ifdef CONFIG_ISM330DHCX_TRIGGER
int ism330dhcx_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);

int ism330dhcx_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ISM330DHCX_ISM330DHCX_H_ */
