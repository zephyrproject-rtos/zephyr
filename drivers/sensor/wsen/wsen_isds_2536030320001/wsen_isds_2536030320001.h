/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_ISDS_2536030320001_WSEN_ISDS_2536030320001_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_ISDS_2536030320001_WSEN_ISDS_2536030320001_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_ISDS_2536030320001_hal.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct isds_2536030320001_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last acceleration samples */
	int16_t acceleration_x;
	int16_t acceleration_y;
	int16_t acceleration_z;

	/* Last angular rate samples */
	int32_t rate_x;
	int32_t rate_y;
	int32_t rate_z;

	/* Last temperature sample */
	int16_t temperature;

	ISDS_accOutputDataRate_t accel_odr;
	ISDS_gyroOutputDataRate_t gyro_odr;

	ISDS_accFullScale_t accel_range;
	ISDS_gyroFullScale_t gyro_range;

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
	const struct device *dev;

	/* Callback for interrupts */
	struct gpio_callback drdy_interrupt_cb;

#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
	struct gpio_callback events_interrupt_cb;
#endif

	/* Registered trigger handlers */
	sensor_trigger_handler_t accel_data_ready_handler;
	sensor_trigger_handler_t gyro_data_ready_handler;
	sensor_trigger_handler_t temp_data_ready_handler;
	sensor_trigger_handler_t single_tap_handler;
	sensor_trigger_handler_t double_tap_handler;
	sensor_trigger_handler_t freefall_handler;
	sensor_trigger_handler_t delta_handler;

	const struct sensor_trigger *accel_data_ready_trigger;
	const struct sensor_trigger *gyro_data_ready_trigger;
	const struct sensor_trigger *temp_data_ready_trigger;
	const struct sensor_trigger *single_tap_trigger;
	const struct sensor_trigger *double_tap_trigger;
	const struct sensor_trigger *freefall_trigger;
	const struct sensor_trigger *delta_trigger;

#if defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(drdy_thread_stack, CONFIG_WSEN_ISDS_2536030320001_THREAD_STACK_SIZE);
	struct k_thread drdy_thread;
	struct k_sem drdy_sem;
#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
	K_KERNEL_STACK_MEMBER(events_thread_stack,
			      CONFIG_WSEN_ISDS_2536030320001_THREAD_STACK_SIZE);
	struct k_thread events_thread;
	struct k_sem events_sem;
#endif
#elif defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD)
	struct k_work drdy_work;
#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
	struct k_work events_work;
#endif
#endif
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */
};

struct isds_2536030320001_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	/* Output data rates */
	ISDS_accOutputDataRate_t accel_odr;
	ISDS_gyroOutputDataRate_t gyro_odr;

	/* Measurement ranges (full scale) */
	uint8_t accel_range;
	uint16_t gyro_range;

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
	/* Interrupt pin (used for data-ready, tap, free-fall, delta/wake-up) */
	const struct gpio_dt_spec events_interrupt_gpio;
	const struct gpio_dt_spec drdy_interrupt_gpio;

#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold;
	uint8_t tap_axis_enable[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */
#ifdef CONFIG_WSEN_ISDS_2536030320001_FREEFALL
	uint8_t freefall_duration;
	ISDS_freeFallThreshold_t freefall_threshold;
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL */
#ifdef CONFIG_WSEN_ISDS_2536030320001_DELTA
	uint8_t delta_threshold;
	uint8_t delta_duration;
#endif /* CONFIG_WSEN_ISDS_2536030320001_DELTA */
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */
};

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
int isds_2536030320001_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler);

int isds_2536030320001_init_interrupt(const struct device *dev);
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_ISDS_2536030320001_WSEN_ISDS_2536030320001_H_ */
