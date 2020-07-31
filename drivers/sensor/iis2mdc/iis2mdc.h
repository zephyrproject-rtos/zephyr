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

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include "iis2mdc_reg.h"

union axis3bit16_t {
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

union axis1bit16_t {
	int16_t i16bit;
	uint8_t u8bit[2];
};

struct iis2mdc_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_IIS2MDC_TRIGGER
	const char *drdy_port;
	gpio_pin_t drdy_pin;
	gpio_dt_flags_t drdy_flags;
#endif  /* CONFIG_IIS2MDC_TRIGGER */
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
struct iis2mdc_data {
	struct device *bus;
	uint16_t i2c_addr;
	int16_t mag[3];
	int32_t temp_sample;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_IIS2MDC_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;
	struct device *dev;

#if defined(CONFIG_IIS2MDC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2MDC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif  /* CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_IIS2MDC_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
};

int iis2mdc_spi_init(struct device *dev);
int iis2mdc_i2c_init(struct device *dev);

#ifdef CONFIG_IIS2MDC_TRIGGER
int iis2mdc_init_interrupt(struct device *dev);
int iis2mdc_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2MDC_TRIGGER */

#endif /* __MAG_IIS2MDC_H */
