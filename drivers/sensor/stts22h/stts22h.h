/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_
#define ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "stts22h_reg.h"

/* ODR constants */
#define ODR_1HZ		0x00
#define ODR_25HZ	0x01
#define ODR_50HZ	0x02
#define	ODR_100HZ	0x03
#define ODR_200HZ	0x04

struct stts22h_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
	int (*bus_init)(const struct device *dev);
#ifdef CONFIG_STTS22H_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct stts22h_data {
	const struct device *dev;
	int16_t sample_temp;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#endif

#ifdef CONFIG_STTS22H_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *thsld_trigger;
	sensor_trigger_handler_t thsld_handler;

#if defined(CONFIG_STTS22H_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_STTS22H_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_STTS22H_TRIGGER */
};

int stts22h_i2c_init(const struct device *dev);

#ifdef CONFIG_STTS22H_TRIGGER
int stts22h_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int stts22h_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_ */
