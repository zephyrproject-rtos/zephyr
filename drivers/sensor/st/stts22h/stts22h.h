/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_
#define ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "stts22h_reg.h"

#include <zephyr/drivers/i2c.h>

struct stts22h_config {
	stmdev_ctx_t ctx;
	const struct i2c_dt_spec i2c;
#ifdef CONFIG_STTS22H_TRIGGER
	const struct gpio_dt_spec int_gpio;
#endif
	uint8_t temp_hi;
	uint8_t temp_lo;
	uint8_t odr;
};

struct stts22h_data {
	const struct device *dev;
	int16_t sample_temp;

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

#ifdef CONFIG_STTS22H_TRIGGER
int stts22h_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int stts22h_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_STTS22H_STTS22H_H_ */
