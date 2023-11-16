/* ST Microelectronics LPS2XDF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheets:
 * https://www.st.com/resource/en/datasheet/lps22df.pdf
 * https://www.st.com/resource/en/datasheet/lps28dfw.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS2XDF_LPS2XDF_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS2XDF_LPS2XDF_H_

#include <stdint.h>
#include <stmemsc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps28dfw)
#include "lps28dfw_reg.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps22df)
#include "lps22df_reg.h"
#endif

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/sensor.h>

#define LPS2XDF_SWRESET_WAIT_TIME_US 50

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
	#define ON_I3C_BUS(cfg) (cfg->i3c.bus != NULL)
#else
	#define ON_I3C_BUS(cfg) (false)
#endif

typedef int32_t (*api_lps2xdf_mode_set_odr_raw)(const struct device *dev, uint8_t odr);
typedef int32_t (*api_lps2xdf_sample_fetch)(const struct device *dev, enum sensor_channel chan);
typedef void (*api_lps2xdf_handle_interrupt)(const struct device *dev);
#ifdef CONFIG_LPS2XDF_TRIGGER
typedef int (*api_lps2xdf_trigger_set)(const struct device *dev,
				       const struct sensor_trigger *trig,
				       sensor_trigger_handler_t handler);
#endif

struct lps2xdf_chip_api {
	api_lps2xdf_mode_set_odr_raw mode_set_odr_raw;
	api_lps2xdf_sample_fetch sample_fetch;
	api_lps2xdf_handle_interrupt handle_interrupt;
#ifdef CONFIG_LPS2XDF_TRIGGER
	api_lps2xdf_trigger_set trigger_set;
#endif
};


enum sensor_variant {
	DEVICE_VARIANT_LPS22DF = 0,
	DEVICE_VARIANT_LPS28DFW = 1,
};


struct lps2xdf_config {
	stmdev_ctx_t ctx;
	union {
#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i2c))
		const struct i2c_dt_spec i2c;
#endif
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, spi)
		const struct spi_dt_spec spi;
#endif
#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
		struct i3c_device_desc **i3c;
#endif
	} stmemsc_cfg;
	uint8_t odr;
	uint8_t lpf;
	uint8_t avg;
	uint8_t drdy_pulsed;
	bool fs;
#ifdef CONFIG_LPS2XDF_TRIGGER
	struct gpio_dt_spec gpio_int;
	bool trig_enabled;
#endif

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
	struct {
		const struct device *bus;
		const struct i3c_device_id dev_id;
	} i3c;
#endif
	const struct lps2xdf_chip_api *chip_api;
};

struct lps2xdf_data {
	int32_t sample_press;
	int16_t sample_temp;

#ifdef CONFIG_LPS2XDF_TRIGGER
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;
	const struct device *dev;

#if defined(CONFIG_LPS2XDF_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LPS2XDF_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem intr_sem;
#elif defined(CONFIG_LPS2XDF_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LPS2XDF_TRIGGER */

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
	struct i3c_device_desc *i3c_dev;
#endif
};

#ifdef CONFIG_LPS2XDF_TRIGGER
int lps2xdf_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lps2xdf_init_interrupt(const struct device *dev, enum sensor_variant variant);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS2XDF_H_ */
