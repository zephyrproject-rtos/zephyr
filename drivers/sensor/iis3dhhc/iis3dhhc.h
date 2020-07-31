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
	int16_t i16bit[3];
	uint8_t u8bit[6];
};

struct iis3dhhc_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_IIS3DHHC_TRIGGER
	const char *int_port;
	uint8_t int_pin;
	uint8_t int_flags;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_config spi_conf;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const char *gpio_cs_port;
	uint8_t cs_gpio;
#endif
#endif
};

struct iis3dhhc_data {
	struct device *bus;
	int16_t acc[3];

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	stmdev_ctx_t ctx_spi;
#endif

#ifdef CONFIG_IIS3DHHC_TRIGGER
	struct device *gpio;
	uint32_t pin;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;
	struct device *dev;

#if defined(CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS3DHHC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_IIS3DHHC_TRIGGER */
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
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
