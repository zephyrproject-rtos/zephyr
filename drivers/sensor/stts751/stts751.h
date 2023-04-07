/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_
#define ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include "stts751_reg.h"

struct stts751_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
	int (*bus_init)(const struct device *dev);
#ifdef CONFIG_STTS751_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct stts751_data {
	const struct device *dev;
	int16_t sample_temp;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#endif

#ifdef CONFIG_STTS751_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *thsld_trigger;
	sensor_trigger_handler_t thsld_handler;

#if defined(CONFIG_STTS751_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_STTS751_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_STTS751_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_STTS751_TRIGGER */
};

int stts751_i2c_init(const struct device *dev);

#ifdef CONFIG_STTS751_TRIGGER
int stts751_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int stts751_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_ */
