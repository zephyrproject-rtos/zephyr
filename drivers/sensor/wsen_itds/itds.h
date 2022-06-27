/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ITDS_ITDS_H_
#define ZEPHYR_DRIVERS_SENSOR_ITDS_ITDS_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_ITDS_2533020201601.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct itds_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last acceleration samples */
	int16_t acceleration_x;
	int16_t acceleration_y;
	int16_t acceleration_z;

	/* Last temperature sample */
	int16_t temperature;

#ifdef CONFIG_ITDS_TRIGGER
	const struct device *dev;

	/* Callback for interrupts (used for data-ready, tap, free-fall, delta/wake-up) */
	struct gpio_callback interrupt_cb;

	/* Registered trigger handlers */
	sensor_trigger_handler_t data_ready_handler;
	sensor_trigger_handler_t single_tap_handler;
	sensor_trigger_handler_t double_tap_handler;
	sensor_trigger_handler_t freefall_handler;
	sensor_trigger_handler_t delta_handler;

#if defined(CONFIG_ITDS_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ITDS_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem interrupt_sem;
#elif defined(CONFIG_ITDS_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ITDS_TRIGGER */
};

/* Operation mode enumeration used for "op-mode" device tree parameter */
typedef enum {
	itds_op_mode_low_power,
	itds_op_mode_normal,
	itds_op_mode_high_performance
} itds_op_mode_t;

struct itds_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	/* Output data rate */
	ITDS_outputDataRate_t odr;

	/* Operation mode */
	itds_op_mode_t op_mode;

	/* Measurement range (full scale) */
	uint8_t range;

	/* Low-noise mode */
	bool low_noise;

#ifdef CONFIG_ITDS_TRIGGER
	/* Interrupt pin (used for data-ready, tap, free-fall, delta/wake-up) */
	const struct gpio_dt_spec gpio_interrupts;

	/* The sensor's data-ready pin number (0 or 1 - corresponding to INT_0 or INT_1) */
	uint8_t drdy_int;

#ifdef CONFIG_ITDS_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_ITDS_TAP */
#ifdef CONFIG_ITDS_FREEFALL
	uint8_t freefall_duration;
	ITDS_FreeFallThreshold_t freefall_threshold;
#endif /* CONFIG_ITDS_FREEFALL */
#ifdef CONFIG_ITDS_DELTA
	uint8_t delta_threshold;
	uint8_t delta_duration;
	int8_t delta_offsets[3];
	uint8_t delta_offset_weight;
#endif /* CONFIG_ITDS_DELTA */
#endif /* CONFIG_ITDS_TRIGGER */
};

#ifdef CONFIG_ITDS_TRIGGER
int itds_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler);

int itds_init_interrupt(const struct device *dev);
#endif /* CONFIG_ITDS_TRIGGER */

int itds_spi_init(const struct device *dev);
int itds_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ITDS_ITDS_H_ */
