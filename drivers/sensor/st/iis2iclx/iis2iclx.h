/* ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2iclx.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS2ICLX_IIS2ICLX_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS2ICLX_IIS2ICLX_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "iis2iclx_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define IIS2ICLX_EN_BIT					0x01
#define IIS2ICLX_DIS_BIT					0x00

/* Accel sensor sensitivity grain is 15 ug/LSB */
#define GAIN_UNIT_XL				(15LL)

struct iis2iclx_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t odr;
	uint8_t range;
#ifdef CONFIG_IIS2ICLX_TRIGGER
	bool trig_enabled;
	uint8_t int_pin;
	const struct gpio_dt_spec gpio_drdy;
#endif /* CONFIG_IIS2ICLX_TRIGGER */
};

#define IIS2ICLX_SHUB_MAX_NUM_SLVS			2

struct iis2iclx_data {
	const struct device *dev;
	int16_t acc[2];
	uint32_t acc_gain;
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
	int temp_sample;
#endif
#if defined(CONFIG_IIS2ICLX_SENSORHUB)
	uint8_t ext_data[2][6];
	uint16_t magn_gain;

	struct hts221_data {
		int16_t x0;
		int16_t x1;
		int16_t y0;
		int16_t y1;
	} hts221;

	bool shub_inited;
	uint8_t num_ext_dev;
	uint8_t shub_ext[IIS2ICLX_SHUB_MAX_NUM_SLVS];
#endif /* CONFIG_IIS2ICLX_SENSORHUB */

	uint16_t accel_freq;
	uint8_t accel_fs;

#ifdef CONFIG_IIS2ICLX_TRIGGER
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_drdy_temp;
	const struct sensor_trigger *trig_drdy_temp;

#if defined(CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2ICLX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2ICLX_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_IIS2ICLX_TRIGGER */
};

#if defined(CONFIG_IIS2ICLX_SENSORHUB)
int iis2iclx_shub_init(const struct device *dev);
int iis2iclx_shub_fetch_external_devs(const struct device *dev);
int iis2iclx_shub_get_idx(const struct device *dev, enum sensor_channel type);
int iis2iclx_shub_config(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val);
#endif /* CONFIG_IIS2ICLX_SENSORHUB */

#ifdef CONFIG_IIS2ICLX_TRIGGER
int iis2iclx_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);

int iis2iclx_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS2ICLX_IIS2ICLX_H_ */
