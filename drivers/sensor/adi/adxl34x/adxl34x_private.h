/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_PRIVATE_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_PRIVATE_H_

#define DT_DRV_COMPAT adi_adxl34x

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/sys/util_macro.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#include "adxl34x_reg.h"

/**
 * @brief Device data for each adxl34x device instance
 *
 * The data in this structure can (and will) change runtime.
 *
 * @struct adxl34x_dev_data
 */
struct adxl34x_dev_data {
	uint64_t timestamp;
#if CONFIG_ADXL34X_ADXL345_COMPATIBLE
	int16_t accel_x[ADXL34X_FIFO_SIZE];
	int16_t accel_y[ADXL34X_FIFO_SIZE];
	int16_t accel_z[ADXL34X_FIFO_SIZE];
	uint8_t sample_number;
#else
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
#endif /* CONFIG_ADXL34X_ADXL345_COMPATIBLE */
	struct adxl34x_cfg cfg;
#ifdef CONFIG_ADXL34X_TRIGGER
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;   /**< Callback to user app */
	sensor_trigger_handler_t motion_event_handler; /**< Callback to user app */
	const struct sensor_trigger *data_ready_trigger;
	const struct sensor_trigger *tap_trigger;
	const struct sensor_trigger *double_tap_trigger;
	const struct sensor_trigger *freefall_trigger;
	const struct sensor_trigger *motion_trigger;
	const struct sensor_trigger *stationary_trigger;
#endif /* CONFIG_ADXL34X_TRIGGER */
#ifdef CONFIG_ADXL34X_ASYNC_API
	struct k_work work;
	const struct device *dev;
	struct rtio_iodev_sqe *iodev_sqe;
#endif /* CONFIG_ADXL34X_ASYNC_API */
};

/**
 * @brief Device (static) configuration for each adxl34x device instance
 *
 * The data in this structure is static can not change runtime. It contains configuration from the
 * device tree, and function pointers to read and write to the device (using i2c or spi).
 *
 * @struct adxl34x_dev_config
 */
struct adxl34x_dev_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
	struct gpio_dt_spec gpio_int1;
	uint8_t dt_int_pin;
	uint8_t dt_packet_size;
	enum adxl34x_accel_range dt_range;
	enum adxl34x_accel_freq dt_rate;

	int (*bus_init)(const struct device *dev);
	int (*bus_write)(const struct device *dev, uint8_t reg_addr, uint8_t reg_data);
	int (*bus_read)(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data);
	int (*bus_read_buf)(const struct device *dev, uint8_t reg_addr, uint8_t *rx_buf,
			    uint8_t size);
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_PRIVATE_H_ */
