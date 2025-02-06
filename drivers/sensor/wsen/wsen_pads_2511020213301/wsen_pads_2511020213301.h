/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_PADS_2511020213301_WSEN_PADS_2511020213301_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_PADS_2511020213301_WSEN_PADS_2511020213301_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_PADS_2511020213301_hal.h"
#include <zephyr/drivers/sensor/wsen_pads_2511020213301.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct pads_2511020213301_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last pressure sample */
	int32_t pressure;

	/* Last temperature sample */
	int16_t temperature;

	PADS_outputDataRate_t sensor_odr;

#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER
	const struct device *dev;

	struct gpio_callback interrupt_cb;

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	sensor_trigger_handler_t pressure_high_trigger_handler;
	sensor_trigger_handler_t pressure_low_trigger_handler;
	const struct sensor_trigger *pressure_high_trigger;
	const struct sensor_trigger *pressure_low_trigger;
#else
	sensor_trigger_handler_t data_ready_trigger_handler;
	const struct sensor_trigger *data_ready_trigger;
#endif /*  CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

#if defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_WSEN_PADS_2511020213301_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER */
};

struct pads_2511020213301_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	/* Output data rate */
	const PADS_outputDataRate_t odr;

	const PADS_powerMode_t configuration;

	const PADS_state_t alpf;

	const PADS_filterConf_t alpf_configuration;
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	const uint16_t threshold;
#endif /*  CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */
#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER
	/* Interrupt pin */
	const struct gpio_dt_spec interrupt_gpio;
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER */
};

#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER
int pads_2511020213301_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler);

int pads_2511020213301_init_interrupt(const struct device *dev);

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
int pads_2511020213301_threshold_set(const struct device *dev,
				     const struct sensor_value *threshold);

int pads_2511020213301_threshold_get(const struct device *dev, struct sensor_value *threshold);

int pads_2511020213301_reference_point_set(const struct device *dev,
					   const struct sensor_value *reference_point);

int pads_2511020213301_reference_point_get(const struct device *dev,
					   struct sensor_value *reference_point);

#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER */

int pads_2511020213301_spi_init(const struct device *dev);
int pads_2511020213301_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_PADS_2511020213301_WSEN_PADS_2511020213301_H_ */
