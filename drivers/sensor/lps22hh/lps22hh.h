/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_

#include <stdint.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <gpio.h>
#include <sensor.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include "lps22hh_reg.h"

union axis1bit32_t {
	s32_t i32bit;
	u8_t u8bit[4];
};

union axis1bit16_t {
	s16_t i16bit;
	u8_t u8bit[2];
};

struct lps22hh_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_LPS22HH_TRIGGER
	const char *drdy_port;
	u8_t drdy_pin;
#endif
#ifdef DT_ST_LPS22HH_BUS_I2C
	u16_t i2c_slv_addr;
#elif DT_ST_LPS22HH_BUS_SPI
	struct spi_config spi_conf;
#if defined(DT_INST_0_ST_LPS22HH_CS_GPIO_CONTROLLER)
	const char *gpio_cs_port;
	u8_t cs_gpio;
#endif
#endif
};

struct lps22hh_data {
	struct device *bus;
	s32_t sample_press;
	s16_t sample_temp;

	stmdev_ctx_t *ctx;

#ifdef DT_ST_LPS22HH_BUS_I2C
	stmdev_ctx_t ctx_i2c;
#elif DT_ST_LPS22HH_BUS_SPI
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_LPS22HH_TRIGGER
	struct device *gpio;
	u32_t pin;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LPS22HH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_LPS22HH_TRIGGER */
#if defined(DT_INST_0_ST_LPS22HH_CS_GPIO_CONTROLLER)
	struct spi_cs_control cs_ctrl;
#endif
};

int lps22hh_i2c_init(struct device *dev);
int lps22hh_spi_init(struct device *dev);

#ifdef CONFIG_LPS22HH_TRIGGER
int lps22hh_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lps22hh_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22HH_LPS22HH_H_ */
