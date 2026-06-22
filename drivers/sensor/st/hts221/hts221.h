/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_
#define ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <stmemsc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include "hts221_reg.h"

#define HTS221_AUTOINCREMENT_ADDR      BIT(7)

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct hts221_data {
	int16_t rh_sample;
	int16_t t_sample;

	uint8_t h0_rh_x2;
	uint8_t h1_rh_x2;
	uint16_t t0_degc_x8;
	uint16_t t1_degc_x8;
	int16_t h0_t0_out;
	int16_t h1_t0_out;
	int16_t t0_out;
	int16_t t1_out;

#ifdef CONFIG_HTS221_TRIGGER
	const struct device *dev;
	struct gpio_callback drdy_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_HTS221_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem drdy_sem;
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_HTS221_TRIGGER */
};

struct hts221_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;

#ifdef CONFIG_HTS221_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	const struct gpio_dt_spec gpio_int;
#endif /* CONFIG_HTS221_TRIGGER */
};

#ifdef CONFIG_HTS221_TRIGGER
int hts221_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int hts221_init_interrupt(const struct device *dev);
#endif /* CONFIG_HTS221_TRIGGER */

int hts221_spi_init(const struct device *dev);
int hts221_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_ */
