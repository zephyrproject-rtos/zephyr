/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_WSEN_TIDS_2521020222501_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_WSEN_TIDS_2521020222501_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_TIDS_2521020222501_hal.h"
#include <zephyr/drivers/sensor/wsen_tids_2521020222501.h>

#include <zephyr/drivers/i2c.h>

struct tids_2521020222501_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last temperature sample */
	int16_t temperature;

	uint8_t sensor_odr;

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	const struct device *dev;

	/* Callback for high/low limit interrupts */
	struct gpio_callback interrupt_cb;

	int32_t sensor_high_threshold;
	int32_t sensor_low_threshold;

	sensor_trigger_handler_t temperature_high_handler;
	sensor_trigger_handler_t temperature_low_handler;

	const struct sensor_trigger *temperature_high_trigger;
	const struct sensor_trigger *temperature_low_trigger;

#if defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_WSEN_TIDS_2521020222501_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER */
};

struct tids_2521020222501_config {
	union {
		const struct i2c_dt_spec i2c;
	} bus_cfg;

	/* Output data rate */
	const uint8_t odr;

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	/* Interrupt pin used for high and low limit interrupt events */
	const struct gpio_dt_spec interrupt_gpio;

	/* High temperature interrupt threshold */
	const int32_t high_threshold;

	/* Low temperature interrupt threshold */
	const int32_t low_threshold;
#endif
};

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
int tids_2521020222501_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler);

int tids_2521020222501_threshold_upper_set(const struct device *dev,
					   const struct sensor_value *thresh_value);

int tids_2521020222501_threshold_upper_get(const struct device *dev,
					   struct sensor_value *thresh_value);

int tids_2521020222501_threshold_lower_set(const struct device *dev,
					   const struct sensor_value *thresh_value);

int tids_2521020222501_threshold_lower_get(const struct device *dev,
					   struct sensor_value *thresh_value);

int tids_2521020222501_init_interrupt(const struct device *dev);
#endif

int tids_2521020222501_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_WSEN_TIDS_2521020222501_H_ */
