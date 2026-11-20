/* ST Microelectronics LIS2DU12 / MIS2DU12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2025 8tronix GmbH
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2du12.pdf
 * https://www.st.com/resource/en/datasheet/mis2du12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DU12_LIS2DU12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DU12_LIS2DU12_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <stmemsc.h>
#include "lis2du12_reg.h"

#define LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(bus)                                                  \
	(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lis2du12, bus) ||                                     \
	 DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_mis2du12, bus))

#if LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define LIS2DU12_EN_BIT  0x01
#define LIS2DU12_DIS_BIT 0x00

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define GAIN_UNIT_XL (61LL)

/* Wake-up duration raw grain is ODR_times (sample periods) */
#define LIS2DU12_WAKUP_DUR_SAMPLES_MAX 15

#define SENSOR_G_DOUBLE (SENSOR_G / 1000000.0)

/*
 * Check https://github.com/STMicroelectronics/MIS2DU12-PID/blob/master/mis2du12_reg.h
 * If MIS2DU12 uses the same WHO_AM_I as LIS2DU12 (0x45), set this to LIS2DU12_ID.
 */
#ifndef MIS2DU12_ID
#define MIS2DU12_ID 0x45
#endif

enum lis2du12_chip_type {
	LIS2DU12_CHIP_LIS2DU12,
	LIS2DU12_CHIP_MIS2DU12,
};

struct lis2du12_config {
	stmdev_ctx_t ctx;
	union {
#if LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if LIS2DU12_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	enum lis2du12_chip_type chip_type;
	uint8_t accel_pm;
	uint8_t accel_odr;
	uint8_t accel_range;
	uint8_t drdy_pulsed;
#ifdef CONFIG_LIS2DU12_TRIGGER
	const struct gpio_dt_spec int1_gpio;
	const struct gpio_dt_spec int2_gpio;
	uint8_t drdy_pin;
	uint8_t delta_pin;
	bool trig_enabled;
#endif /* CONFIG_LIS2DU12_TRIGGER */
};

union samples {
	uint8_t raw[6];
	struct {
		int16_t axis[3];
	};
} __aligned(2);

struct lis2du12_data {
	const struct device *dev;
	int16_t acc[3];
	uint32_t acc_gain; /* in ug/LSB */
	uint8_t accel_fs;  /* in g */

#ifdef CONFIG_LIS2DU12_TRIGGER
	atomic_t trig_flags;

	struct gpio_callback gpio_cb[2];
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *trig_drdy_acc;
	sensor_trigger_handler_t handler_delta_xyz_acc;
	const struct sensor_trigger *trig_delta_xyz_acc;

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DU12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LIS2DU12_TRIGGER */
};

/* Header + payload used by the Read and Decode API (CONFIG_SENSOR_ASYNC_API) */
struct lis2du12_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t accel_fs;
	} header;
	int16_t acc[3];
} __attribute__((__packed__));

#ifdef CONFIG_SENSOR_ASYNC_API
void lis2du12_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int lis2du12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif

#ifdef CONFIG_LIS2DU12_TRIGGER
int lis2du12_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int lis2du12_init_interrupt(const struct device *dev);

int lis2du12_accel_set_wake_th(const struct device *dev, const struct sensor_value *val);

int lis2du12_accel_set_wake_dur(const struct device *dev, const struct sensor_value *val);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DU12_LIS2DU12_H_ */
