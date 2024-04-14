/* sensor_lps25hb.h - header file for LPS22HB pressure and temperature
 * sensor driver
 */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_

#include <stdint.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lps22hb_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct lps22hb_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t odr;
	bool low_pass_enabled;
	lps22hb_lpfp_t filter_mode;
#ifdef CONFIG_LPS22HB_TRIGGER
	struct gpio_dt_spec gpio_int;
#endif
};

struct lps22hb_data {
	int32_t sample_press;
	int16_t sample_temp;

#ifdef CONFIG_LPS22HB_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;
	const struct device *dev;

#if defined(CONFIG_LPS22HB_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LPS22HB_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem intr_sem;
#elif defined(CONFIG_LPS22HB_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LPS22HB_TRIGGER */
};

#ifdef CONFIG_LPS22HB_TRIGGER
int lps22hb_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lps22hb_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_ */
