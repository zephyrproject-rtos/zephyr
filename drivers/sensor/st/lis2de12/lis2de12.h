/* ST Microelectronics LIS2DE12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2de12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DE12_LIS2DE12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DE12_LIS2DE12_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lis2de12_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define LIS2DE12_EN_BIT  0x01
#define LIS2DE12_DIS_BIT 0x00

#define SENSOR_G_DOUBLE	(SENSOR_G / 1000000.0)

struct lis2de12_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t accel_pm;
	uint8_t accel_odr;
	uint8_t accel_range;
	uint8_t drdy_pulsed;
#ifdef CONFIG_LIS2DE12_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	bool trig_enabled;
#endif /* CONFIG_LIS2DE12_TRIGGER */
};

union samples {
	uint8_t raw[6];
	struct {
		int16_t axis[3];
	};
} __aligned(2);

struct lis2de12_data {
	const struct device *dev;
	int16_t acc[3];
	int16_t temp_sample;
	uint32_t acc_gain;
	uint8_t accel_freq;
	uint8_t accel_fs;

#ifdef CONFIG_LIS2DE12_TRIGGER
	struct gpio_dt_spec *drdy_gpio;

	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;

#if defined(CONFIG_LIS2DE12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DE12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DE12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LIS2DE12_TRIGGER */
};

#ifdef CONFIG_LIS2DE12_TRIGGER
int lis2de12_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int lis2de12_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DE12_LIS2DE12_H_ */
