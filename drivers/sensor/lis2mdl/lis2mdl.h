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
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

union axis1bit16_t {
	int16_t i16bit;
	uint8_t u8bit[2];
};

struct lis2mdl_config {
	char *master_dev_name;
	int (*bus_init)(const struct device *dev);
#ifdef CONFIG_LIS2MDL_TRIGGER
	char *gpio_name;
	uint32_t gpio_pin;
	uint8_t gpio_flags;
#endif  /* CONFIG_LIS2MDL_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	uint16_t i2c_slv_addr;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_config spi_conf;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const char *gpio_cs_port;
	uint8_t cs_gpio;
	uint8_t cs_gpio_flags;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

/* Sensor data */
struct lis2mdl_data {
	const struct device *dev;
	const struct device *bus;
	uint16_t i2c_addr;
	int16_t mag[3];
	int32_t temp_sample;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2MDL_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif  /* CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_LIS2MDL_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
};

int lis2mdl_spi_init(const struct device *dev);
int lis2mdl_i2c_init(const struct device *dev);

#ifdef CONFIG_LIS2MDL_TRIGGER
int lis2mdl_init_interrupt(const struct device *dev);
int lis2mdl_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2MDL_TRIGGER */

#endif /* __MAG_LIS2MDL_H */
