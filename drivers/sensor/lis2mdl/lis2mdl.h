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

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include "lis2mdl_reg.h"

union axis3bit16_t {
	s16_t i16bit[3];
	u8_t u8bit[6];
};

union axis1bit16_t {
	s16_t i16bit;
	u8_t u8bit[2];
};

struct lis2mdl_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_LIS2MDL_TRIGGER
	char *gpio_name;
	u32_t gpio_pin;
	u8_t gpio_flags;
#endif  /* CONFIG_LIS2MDL_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	u16_t i2c_slv_addr;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_config spi_conf;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const char *gpio_cs_port;
	u8_t cs_gpio;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

/* Sensor data */
struct lis2mdl_data {
	struct device *bus;
	u16_t i2c_addr;
	s16_t mag[3];
	s32_t temp_sample;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS2MDL_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif  /* CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_LIS2MDL_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
};

int lis2mdl_spi_init(struct device *dev);
int lis2mdl_i2c_init(struct device *dev);

#ifdef CONFIG_LIS2MDL_TRIGGER
int lis2mdl_init_interrupt(struct device *dev);
int lis2mdl_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2MDL_TRIGGER */

#endif /* __MAG_LIS2MDL_H */
