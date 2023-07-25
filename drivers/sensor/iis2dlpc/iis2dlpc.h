/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_
#define ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <stmemsc.h>
#include "iis2dlpc_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/* Return ODR reg value based on data rate set */
#define IIS2DLPC_ODR_TO_REG(_odr) \
	((_odr <= 1) ? IIS2DLPC_XL_ODR_1Hz6_LP_ONLY : \
	 (_odr <= 12) ? IIS2DLPC_XL_ODR_12Hz5 : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

/* FS reg value from Full Scale */
#define IIS2DLPC_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

/* Acc Gain value in ug/LSB in High Perf mode */
#define IIS2DLPC_FS_2G_GAIN		244
#define IIS2DLPC_FS_4G_GAIN		488
#define IIS2DLPC_FS_8G_GAIN		976
#define IIS2DLPC_FS_16G_GAIN		1952

#define IIS2DLPC_SHFT_GAIN_NOLP1	2
#define IIS2DLPC_ACCEL_GAIN_DEFAULT_VAL IIS2DLPC_FS_2G_GAIN
#define IIS2DLPC_FS_TO_GAIN(_fs, _lp1) \
		(IIS2DLPC_FS_2G_GAIN << ((_fs) + (_lp1)))

/* shift value for power mode */
#define IIS2DLPC_SHIFT_PM1		4
#define IIS2DLPC_SHIFT_PMOTHER		2

/**
 * struct iis2dlpc_dev_config - iis2dlpc hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @irq_dev_name: Pointer to GPIO PORT identifier.
 * @irq_pin: GPIO pin number connected to sensor int pin.
 * @drdy_int: Sensor drdy int (int1/int2).
 */
struct iis2dlpc_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	iis2dlpc_mode_t pm;
	uint8_t range;
#ifdef CONFIG_IIS2DLPC_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	uint8_t drdy_int;
#ifdef CONFIG_IIS2DLPC_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_IIS2DLPC_TAP */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
};

/* sensor data */
struct iis2dlpc_data {
	const struct device *dev;
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;

#ifdef CONFIG_IIS2DLPC_TRIGGER
	const struct device *gpio;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trig;
#ifdef CONFIG_IIS2DLPC_TAP
	sensor_trigger_handler_t tap_handler;
	const struct sensor_trigger *tap_trig;
	sensor_trigger_handler_t double_tap_handler;
	const struct sensor_trigger *double_tap_trig;
#endif /* CONFIG_IIS2DLPC_TAP */
#ifdef CONFIG_IIS2DLPC_ACTIVITY
	sensor_trigger_handler_t activity_handler;
	const struct sensor_trigger *activity_trig;
#endif /* CONFIG_IIS2DLPC_ACTIVITY */
#if defined(CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_IIS2DLPC_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_IIS2DLPC_TRIGGER */
};

#ifdef CONFIG_IIS2DLPC_TRIGGER
int iis2dlpc_init_interrupt(const struct device *dev);
int iis2dlpc_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_IIS2DLPC_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_IIS2DLPC_IIS2DLPC_H_ */
