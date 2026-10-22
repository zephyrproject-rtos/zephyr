/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICP201XXX_ICP101XXX_H_
#define ZEPHYR_DRIVERS_SENSOR_ICP201XXX_ICP101XXX_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include "Icp201xx.h"
#include "Icp201xxSerif.h"

union icp201xx_bus {
#if CONFIG_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*icp201xx_reg_read_fn)(const union icp201xx_bus *bus, uint8_t reg, uint8_t *buf,
				    uint32_t size);
typedef int (*icp201xx_reg_write_fn)(const union icp201xx_bus *bus, uint8_t reg, uint8_t *buf,
				     uint32_t size);

struct icp201xx_bus_io {
	icp201xx_reg_read_fn read;
	icp201xx_reg_write_fn write;
};

extern const struct icp201xx_bus_io icp201xx_bus_io_spi;
extern const struct icp201xx_bus_io icp201xx_bus_io_i2c;

struct icp201xx_data {
	int32_t raw_pressure;
	int32_t raw_temperature;
	icp201xx_op_mode_t op_mode;
	inv_icp201xx_t icp_device;

#ifdef CONFIG_ICP201XX_TRIGGER
	struct sensor_value pressure_change;
	struct sensor_value pressure_threshold;
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	sensor_trigger_handler_t delta_handler;
	sensor_trigger_handler_t threshold_handler;
	const struct sensor_trigger *drdy_trigger;
	const struct sensor_trigger *delta_trigger;
	const struct sensor_trigger *threshold_trigger;
	struct k_mutex mutex;
#ifdef CONFIG_ICP201XX_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICP201XX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICP201XX_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
#endif
};

struct icp201xx_config {
	union icp201xx_bus bus;
	const struct icp201xx_bus_io *bus_io;
	icp201xx_if_t if_mode;
	struct gpio_dt_spec gpio_int;
	icp201xx_op_mode_t op_mode;
	uint8_t drive_strength;
};

int icp201xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int icp201xx_trigger_init(const struct device *dev);

void icp201xx_mutex_lock(const struct device *dev);
void icp201xx_mutex_unlock(const struct device *dev);
void inv_icp201xx_app_warmup(inv_icp201xx_t *icp_device, icp201xx_op_mode_t op_mode,
			     icp201xx_meas_mode_t meas_mode);
#endif
