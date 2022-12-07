/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_WSEN_HIDS_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_WSEN_HIDS_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_HIDS_2523020210001.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct hids_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last humidity sample */
	uint16_t humidity;

	/* Last temperature sample */
	int16_t temperature;

#ifdef CONFIG_WSEN_HIDS_TRIGGER
	const struct device *dev;
	struct gpio_callback data_ready_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_WSEN_HIDS_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_WSEN_HIDS_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem drdy_sem;
#elif defined(CONFIG_WSEN_HIDS_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_WSEN_HIDS_TRIGGER */
};

struct hids_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	/* Output data rate */
	HIDS_outputDataRate_t odr;

#ifdef CONFIG_WSEN_HIDS_TRIGGER
	/* Data-ready interrupt pin */
	const struct gpio_dt_spec gpio_drdy;
#endif /* CONFIG_WSEN_HIDS_TRIGGER */
};

#ifdef CONFIG_WSEN_HIDS_TRIGGER
int hids_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler);

int hids_init_interrupt(const struct device *dev);
#endif /* CONFIG_WSEN_HIDS_TRIGGER */

int hids_spi_init(const struct device *dev);
int hids_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_HIDS_WSEN_HIDS_H_ */
