/* ST Microelectronics LIS2MDL 3-axis magnetometer sensor
 *
 * Copyright (c) 2018-2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2mdl.pdf
 */

#ifndef __MAG_LIS2MDL_H
#define __MAG_LIS2MDL_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lis2mdl_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct lis2mdl_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	bool cancel_offset;
	bool single_mode;
	bool spi_4wires;

#ifdef CONFIG_LIS2MDL_TRIGGER
	bool trig_enabled;
	const struct gpio_dt_spec gpio_drdy;
#endif  /* CONFIG_LIS2MDL_TRIGGER */
};

/* Sensor data */
struct lis2mdl_data {
	const struct device *dev;
	int16_t mag[3];
	int16_t temp_sample;
	struct k_sem fetch_sem;

#ifdef CONFIG_LIS2MDL_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trig_drdy;

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2MDL_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif  /* CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_LIS2MDL_TRIGGER */
};

#ifdef CONFIG_LIS2MDL_TRIGGER
int lis2mdl_init_interrupt(const struct device *dev);
int lis2mdl_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2MDL_TRIGGER */

#endif /* __MAG_LIS2MDL_H */
