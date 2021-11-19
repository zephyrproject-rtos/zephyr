/* ST Microelectronics IIS3DWB 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_

#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include <stmemsc.h>
#include "iis3dwb_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

/* Acc Gain value in ug/LSB in High Perf mode */
#define IIS3DWB_FS_2G_GAIN		61
#define IIS3DWB_FS_4G_GAIN		122
#define IIS3DWB_FS_8G_GAIN		244
#define IIS3DWB_FS_16G_GAIN		488

/**
 * struct iis3dwb_dev_config - iis3dwb hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @irq_dev_name: Pointer to GPIO PORT identifier.
 * @irq_pin: GPIO pin number connecter to sensor int pin.
 * @drdy_int: Sensor drdy int (int1/int2).
 */
struct iis3dwb_config {
	stmdev_ctx_t ctx;
	const struct spi_dt_spec spi;
	uint8_t range;
	uint8_t odr;
	uint8_t filt_type;
	uint8_t filt_cfg;
	uint8_t filt_num;
#ifdef CONFIG_IIS3DWB_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	uint8_t drdy_int;
#endif /* CONFIG_IIS3DWB_TRIGGER */
};

/* sensor data */
struct iis3dwb_data {
	const struct device *dev;
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;

#ifdef CONFIG_IIS3DWB_TRIGGER
	const struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#if defined(CONFIG_IIS3DWB_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS3DWB_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS3DWB_TRIGGER */
};

#ifdef CONFIG_IIS3DWB_TRIGGER
int iis3dwb_init_interrupt(const struct device *dev);
int iis3dwb_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS3DWB_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS3DWB_IIS3DWB_H_ */
