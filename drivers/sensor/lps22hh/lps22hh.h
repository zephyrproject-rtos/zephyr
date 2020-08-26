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
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include "lps22hh_reg.h"

union axis1bit32_t {
	int32_t i32bit;
	uint8_t u8bit[4];
};

union axis1bit16_t {
	int16_t i16bit;
	uint8_t u8bit[2];
};

struct lps22hh_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_LPS22HH_TRIGGER
	const char *drdy_port;
	uint8_t drdy_pin;
	uint8_t drdy_flags;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	uint16_t i2c_slv_addr;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_config spi_conf;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const char *gpio_cs_port;
	uint8_t cs_gpio;
	uint8_t cs_gpio_flags;
#endif
#endif
};

struct lps22hh_data {
	struct device *bus;
	int32_t sample_press;
	int16_t sample_temp;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_LPS22HH_TRIGGER
	struct device *gpio;
	uint32_t pin;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t handler_drdy;
	struct device *dev;

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LPS22HH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LPS22HH_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
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
