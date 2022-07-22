/* ST Microelectronics IIS2MDC 3-axis magnetometer sensor
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2mdc.pdf
 */

#ifndef __MAG_IIS2MDC_H
#define __MAG_IIS2MDC_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include "iis2mdc_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct iis2mdc_dev_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
	};
	int (*bus_init)(const struct device *dev);
#ifdef CONFIG_IIS2MDC_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
#endif  /* CONFIG_IIS2MDC_TRIGGER */
};

/* Sensor data */
struct iis2mdc_data {
	const struct device *dev;
	int16_t mag[3];
	int32_t temp_sample;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_IIS2MDC_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_IIS2MDC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2MDC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif  /* CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_IIS2MDC_TRIGGER */
};

int iis2mdc_spi_init(const struct device *dev);
int iis2mdc_i2c_init(const struct device *dev);

#ifdef CONFIG_IIS2MDC_TRIGGER
int iis2mdc_init_interrupt(const struct device *dev);
int iis2mdc_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2MDC_TRIGGER */

#endif /* __MAG_IIS2MDC_H */
