/* ST Microelectronics LIS2DUX12 3-axis accelerometer driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dux12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_LIS2DUX12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_LIS2DUX12_H_

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stmemsc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_lis2dux12)
#include "lis2dux12_reg.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lis2duxs12)
#include "lis2duxs12_reg.h"
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2dux12, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2duxs12, spi)
#include <zephyr/drivers/spi.h>
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2dux12, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2duxs12, i2c)
#include <zephyr/drivers/i2c.h>
#endif

typedef int32_t (*api_lis2dux12_set_odr_raw)(const struct device *dev, uint8_t odr);
typedef int32_t (*api_lis2dux12_set_range)(const struct device *dev, uint8_t range);
typedef int32_t (*api_lis2dux12_sample_fetch_accel)(const struct device *dev);
#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
typedef int32_t (*api_lis2dux12_sample_fetch_temp)(const struct device *dev);
#endif
#ifdef CONFIG_LIS2DUX12_TRIGGER
typedef void (*api_lis2dux12_handle_interrupt)(const struct device *dev);
typedef int32_t (*api_lis2dux12_init_interrupt)(const struct device *dev);
#endif

struct lis2dux12_chip_api {
	api_lis2dux12_set_odr_raw set_odr_raw;
	api_lis2dux12_set_range	set_range;
	api_lis2dux12_sample_fetch_accel sample_fetch_accel;
#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
	api_lis2dux12_sample_fetch_temp sample_fetch_temp;
#endif
#ifdef CONFIG_LIS2DUX12_TRIGGER
	api_lis2dux12_handle_interrupt handle_interrupt;
	api_lis2dux12_init_interrupt init_interrupt;
#endif
};

struct lis2dux12_config {
	stmdev_ctx_t ctx;
	union {
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2dux12, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2duxs12, i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2dux12, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2duxs12, spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t range;
	uint8_t pm;
	uint8_t odr;
#ifdef CONFIG_LIS2DUX12_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	uint8_t drdy_pin;
	bool trig_enabled;
#endif

	const struct lis2dux12_chip_api *chip_api;
};

struct lis2dux12_data {
	int sample_x;
	int sample_y;
	int sample_z;
	float gain;
	uint8_t range;
	uint8_t odr;

#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
	float sample_temp;
#endif

#ifdef CONFIG_LIS2DUX12_TRIGGER
	struct gpio_dt_spec *drdy_gpio;
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;
	const struct device *dev;

#if defined(CONFIG_LIS2DUX12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DUX12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#elif defined(CONFIG_LIS2DUX12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LIS2DUX12_TRIGGER */
};

#ifdef CONFIG_LIS2DUX12_TRIGGER
int lis2dux12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int lis2dux12_trigger_init(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_LIS2DUX12_H_ */
