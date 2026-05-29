/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <stmemsc.h>
#include "lis2ds12_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/* Return ODR reg value based on data rate set */
#define LIS2DS12_ODR_TO_REG(_odr) \
	((_odr <= 1) ? 1 : \
	((31 - __builtin_clz(_odr / 25))) + 3)

struct lis2ds12_config {
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
	uint8_t pm;
	uint8_t odr;
#ifdef CONFIG_LIS2DS12_TRIGGER
	struct gpio_dt_spec gpio_int;
#endif
};

struct lis2ds12_data {
	int sample_x;
	int sample_y;
	int sample_z;
	float gain;

#ifdef CONFIG_LIS2DS12_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;
	const struct device *dev;

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DS12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LIS2DS12_TRIGGER */
};

#ifdef CONFIG_LIS2DS12_TRIGGER
int lis2ds12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int lis2ds12_trigger_init(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_ */
