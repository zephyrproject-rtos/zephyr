/* ST Microelectronics LPS22DF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22df.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22DF_LPS22DF_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22DF_LPS22DF_H_

#include <stdint.h>
#include <stmemsc.h>
#include "lps22df_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
#include <zephyr/drivers/i3c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	#define ON_I3C_BUS(cfg) (cfg->i3c.bus != NULL)
#else
	#define ON_I3C_BUS(cfg) (false)
#endif

struct lps22df_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		struct i3c_device_desc **i3c;
#endif
	} stmemsc_cfg;
	uint8_t odr;
	uint8_t lpf;
	uint8_t avg;
	uint8_t drdy_pulsed;
#ifdef CONFIG_LPS22DF_TRIGGER
	struct gpio_dt_spec gpio_int;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct {
		const struct device *bus;
		const struct i3c_device_id dev_id;
	} i3c;
#endif
};

struct lps22df_data {
	int32_t sample_press;
	int16_t sample_temp;

#ifdef CONFIG_LPS22DF_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;
	const struct device *dev;

#if defined(CONFIG_LPS22DF_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LPS22DF_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem intr_sem;
#elif defined(CONFIG_LPS22DF_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LPS22DF_TRIGGER */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct i3c_device_desc *i3c_dev;
#endif
};

#ifdef CONFIG_LPS22DF_TRIGGER
int lps22df_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lps22df_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22DF_LPS22DF_H_ */
