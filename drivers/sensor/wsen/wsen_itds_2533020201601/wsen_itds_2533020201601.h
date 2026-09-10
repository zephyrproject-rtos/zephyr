/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_ITDS_2533020201601_WSEN_ITDS_2533020201601_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_ITDS_2533020201601_WSEN_ITDS_2533020201601_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <platform.h>

#include <WSEN_ITDS_2533020201601.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct itds_2533020201601_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last acceleration samples */
	int16_t acceleration_x;
	int16_t acceleration_y;
	int16_t acceleration_z;

	/* Last temperature sample */
	int16_t temperature;

	ITDS_outputDataRate_t sensor_odr;

	ITDS_fullScale_t sensor_range;

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER
	const struct device *dev;
	/* Callback for interrupts */
	struct gpio_callback drdy_interrupt_cb;

#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
	struct gpio_callback events_interrupt_cb;
#endif

	/* Registered trigger handlers */
	sensor_trigger_handler_t accel_data_ready_handler;
	sensor_trigger_handler_t temp_data_ready_handler;
	sensor_trigger_handler_t single_tap_handler;
	sensor_trigger_handler_t double_tap_handler;
	sensor_trigger_handler_t freefall_handler;
	sensor_trigger_handler_t delta_handler;

	const struct sensor_trigger *accel_data_ready_trigger;
	const struct sensor_trigger *temp_data_ready_trigger;
	const struct sensor_trigger *single_tap_trigger;
	const struct sensor_trigger *double_tap_trigger;
	const struct sensor_trigger *freefall_trigger;
	const struct sensor_trigger *delta_trigger;

#if defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(drdy_thread_stack, CONFIG_WSEN_ITDS_2533020201601_THREAD_STACK_SIZE);
	struct k_thread drdy_thread;
	struct k_sem drdy_sem;
#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
	K_KERNEL_STACK_MEMBER(events_thread_stack,
			      CONFIG_WSEN_ITDS_2533020201601_THREAD_STACK_SIZE);
	struct k_thread events_thread;
	struct k_sem events_sem;
#endif
#elif defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD)
	struct k_work drdy_work;
#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
	struct k_work events_work;
#endif
#endif
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER */
};

struct itds_2533020201601_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	/* Output data rate */
	const ITDS_outputDataRate_t odr;

	/* Operation mode */
	const ITDS_operatingMode_t op_mode;

	/* Power mode */
	const ITDS_powerMode_t power_mode;

	/* Measurement range (full scale) */
	const uint8_t range;

	/* Low-noise mode */
	const ITDS_state_t low_noise;

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER
	/* Interrupt pins */
	const struct gpio_dt_spec events_interrupt_gpio;
	const struct gpio_dt_spec drdy_interrupt_gpio;

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP
	const uint8_t tap_mode;
	const uint8_t tap_threshold[3];
	const uint8_t tap_shock;
	const uint8_t tap_latency;
	const uint8_t tap_quiet;
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */
#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
	const uint8_t freefall_duration;
	const ITDS_FreeFallThreshold_t freefall_threshold;
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */
#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
	const uint8_t delta_threshold;
	const uint8_t delta_duration;
	const int8_t delta_offsets[3];
	const uint8_t delta_offset_weight;
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER */
};

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER
int itds_2533020201601_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler);

int itds_2533020201601_init_interrupt(const struct device *dev);
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_ITDS_2533020201601_WSEN_ITDS_2533020201601_H_ */
