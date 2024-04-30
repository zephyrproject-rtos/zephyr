/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <stmemsc.h>
#include "lis2dw12_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/* Return ODR reg value based on data rate set */
#define LIS2DW12_ODR_TO_REG(_odr) \
	((_odr <= 1) ? LIS2DW12_XL_ODR_1Hz6_LP_ONLY : \
	 (_odr <= 12) ? LIS2DW12_XL_ODR_12Hz5 : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

/* Return data rate in Hz for given register value */
#define LIS2DW12_REG_TO_ODR(_reg) \
	((_reg == 0) ? 0 : \
	(_reg == 1) ? 1 : \
	(_reg == 2) ? 12 : \
	(_reg > 9) ? 1600 : \
	(1 << (_reg - 3)) * 25)

/* FS reg value from Full Scale */
#define LIS2DW12_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

/* Acc Gain value in ug/LSB in High Perf mode */
#define LIS2DW12_FS_2G_GAIN		244
#define LIS2DW12_FS_4G_GAIN		488
#define LIS2DW12_FS_8G_GAIN		976
#define LIS2DW12_FS_16G_GAIN		1952

#define LIS2DW12_SHFT_GAIN_NOLP1	2
#define LIS2DW12_ACCEL_GAIN_DEFAULT_VAL LIS2DW12_FS_2G_GAIN
#define LIS2DW12_FS_TO_GAIN(_fs, _lp1) \
		(LIS2DW12_FS_2G_GAIN << ((_fs) + (_lp1)))

/* shift value for power mode */
#define LIS2DW12_SHIFT_PM1		4
#define LIS2DW12_SHIFT_PMOTHER		2

/**
 * struct lis2dw12_device_config - lis2dw12 hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @int_pin: Sensor int pin (int1/int2).
 */
struct lis2dw12_device_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	lis2dw12_mode_t pm;
	uint16_t odr;
	uint8_t range;
	uint8_t bw_filt;
	bool low_noise;
	bool hp_filter_path;
	bool hp_ref_mode;
	bool drdy_pulsed;
#ifdef CONFIG_LIS2DW12_TRIGGER
	struct gpio_dt_spec gpio_int;
	uint8_t int_pin;
#ifdef CONFIG_LIS2DW12_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_LIS2DW12_TAP */
#ifdef CONFIG_LIS2DW12_FREEFALL
	uint8_t freefall_duration;
	uint8_t freefall_threshold;
#endif /* CONFIG_LIS2DW12_FREEFALL */
#ifdef CONFIG_LIS2DW12_THRESHOLD
	uint8_t wakeup_duration;
#endif /* CONFIG_LIS2DW12_THRESHOLD */
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

/* sensor data */
struct lis2dw12_data {
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;
	 /* output data rate */
	uint16_t odr;

#ifdef CONFIG_LIS2DW12_TRIGGER
	const struct device *dev;

	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trig;
#ifdef CONFIG_LIS2DW12_TAP
	sensor_trigger_handler_t tap_handler;
	const struct sensor_trigger *tap_trig;
	sensor_trigger_handler_t double_tap_handler;
	const struct sensor_trigger *double_tap_trig;
#endif /* CONFIG_LIS2DW12_TAP */
#ifdef CONFIG_LIS2DW12_THRESHOLD
	sensor_trigger_handler_t threshold_handler;
	const struct sensor_trigger *threshold_trig;
#endif /* CONFIG_LIS2DW12_THRESHOLD */
#ifdef CONFIG_LIS2DW12_FREEFALL
	sensor_trigger_handler_t freefall_handler;
	const struct sensor_trigger *freefall_trig;
#endif /* CONFIG_LIS2DW12_FREEFALL */
#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DW12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

#ifdef CONFIG_LIS2DW12_TRIGGER
int lis2dw12_init_interrupt(const struct device *dev);
int lis2dw12_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2DW12_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_ */
