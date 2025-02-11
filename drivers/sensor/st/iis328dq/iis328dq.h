/* ST Microelectronics IIS328DQ 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis328dq.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS328DQ_IIS328DQ_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS328DQ_IIS328DQ_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <stmemsc.h>
#include "iis328dq_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/**
 * struct iis328dq_dev_config - iis328dq hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @irq_dev_name: Pointer to GPIO PORT identifier.
 * @irq_pin: GPIO pin number connected to sensor int pin.
 * @drdy_int: Sensor drdy int (int1/int2).
 */
struct iis328dq_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t range;
#ifdef CONFIG_IIS328DQ_TRIGGER
	const struct gpio_dt_spec gpio_int1;
	const struct gpio_dt_spec gpio_int2;
	/* interrupt pad to be used for DRDY interrupts, or -1 if not configured */
	int8_t drdy_pad;
#ifdef CONFIG_IIS328DQ_THRESHOLD
	/* interrupt pad to be used for threshold interrupts, or -1 if not configured */
	int8_t threshold_pad;
#endif /* CONFIG_IIS328DQ_THRESHOLD */
#endif /* CONFIG_IIS328DQ_TRIGGER */
};

/* sensor data */
struct iis328dq_data {
	const struct device *dev;
	int16_t acc[3];

	/* sensitivity in mg/LSB */
	uint8_t gain;

#ifdef CONFIG_IIS328DQ_TRIGGER
	struct gpio_callback int1_cb;
	struct gpio_callback int2_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trig;
#ifdef CONFIG_IIS328DQ_THRESHOLD
	sensor_trigger_handler_t threshold_handler;
	const struct sensor_trigger *threshold_trig;
#endif /* CONFIG_IIS328DQ_THRESHOLD */
#if defined(CONFIG_IIS328DQ_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS328DQ_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS328DQ_TRIGGER */
};

#ifdef CONFIG_IIS328DQ_TRIGGER
int iis328dq_init_interrupt(const struct device *dev);
int iis328dq_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS328DQ_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS328DQ_IIS328DQ_H_ */
