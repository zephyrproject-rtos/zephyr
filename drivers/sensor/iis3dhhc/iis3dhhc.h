/* ST Microelectronics IIS3DHHC accelerometer sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dhhc.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS3DHHC_IIS3DHHC_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS3DHHC_IIS3DHHC_H_

#include <stdint.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include "iis3dhhc_reg.h"

union axis3bit16_t {
	s16_t i16bit[3];
	u8_t u8bit[6];
};

struct iis3dhhc_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_IIS3DHHC_TRIGGER
	const char *int_port;
	u8_t int_pin;
#endif
#ifdef DT_ST_IIS3DHHC_BUS_SPI
	struct spi_config spi_conf;
#if defined(DT_INST_0_ST_IIS3DHHC_CS_GPIOS_CONTROLLER)
	const char *gpio_cs_port;
	u8_t cs_gpio;
#endif
#endif
};

struct iis3dhhc_data {
	struct device *bus;
	s16_t acc[3];

	stmdev_ctx_t *ctx;

#ifdef DT_ST_IIS3DHHC_BUS_SPI
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_IIS3DHHC_TRIGGER
	struct device *gpio;
	u32_t pin;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_IIS3DHHC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_IIS3DHHC_TRIGGER */
#if defined(DT_INST_0_ST_IIS3DHHC_CS_GPIOS_CONTROLLER)
	struct spi_cs_control cs_ctrl;
#endif
};

int iis3dhhc_spi_init(struct device *dev);

#ifdef CONFIG_IIS3DHHC_TRIGGER
int iis3dhhc_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int iis3dhhc_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS3DHHC_IIS3DHHC_H_ */
