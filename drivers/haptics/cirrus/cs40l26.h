/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L26_H_
#define ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L26_H_

#include "cs40lxx.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_instance.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Supported devices */
#define CS40L26_DEVID_A1  0x40A260U
#define CS40L26_REVID_A1  0xA1U
#define CS40L27_DEVID_B2  0x40A270U
#define CS40L27_REVID_B2  0xB2U
#define CS40L26_OTPID_MIN 0x1U
#define CS40L26_OTPID_MAX 0xEU

enum { /* Non-deterministic firmware registers */
	CS40L26_REG_PM_TIMER_TIMEOUT_TICKS,
	CS40L26_REG_PM_STDBY_TIMEOUT,
	CS40L26_REG_PM_ACTIVE_TIMEOUT,
	CS40L26_REG_PM_POWER_ON_SEQUENCE,
	CS40L26_REG_VIBEGEN_F0_OTP_STORED,
	CS40L26_REG_VIBEGEN_REDC_OTP_STORED,
	CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE,
	CS40L26_REG_RE_EST_STATUS,
	CS40L26_REG_LOGGER_ENABLE,
	CS40L26_REG_LOGGER_DATA,
	CS40L26_REG_DYNAMIC_F0_ENABLED,
	CS40L26_REG_HALO_STATE,
	CS40L26_REG_SOURCE_ATTENUATION,
	CS40L26_REG_BUZZ_FREQ,
	CS40L26_REG_BUZZ_LEVEL,
	CS40L26_REG_BUZZ_DURATION,
	CS40L26_REG_GPIO_EVENT_BASE,
	CS40L26_REG_F0_EST_REDC,
	CS40L26_REG_F0_EST,
	CS40L26_REG_MAILBOX_QUEUE_WT,
	CS40L26_REG_MAILBOX_QUEUE_RD,
	CS40L26_REG_MAILBOX_STATUS,
	CS40L26_NUM_FIRMWARE_REGISTERS,
};

struct cs40l26_calibration {
	/* Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/* Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

struct cs40l26_sensor {
	bool is_signed;
	uint8_t n;
	uint8_t m;
	struct sensor_value full_scale;
};

struct cs40l26_config {
	LOG_INSTANCE_PTR_DECLARE(log);
	/* Log instance declaration requires blank line. */
	const struct device *const dev;
	const uint32_t dev_id;
	const struct cs40lxx_io_bus io_bus;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec interrupt_gpio;
	const struct gpio_dt_spec *const trigger_gpios;
	const int *const trigger_mapping;
	const uint8_t num_triggers;
	const struct device *flash;
	const off_t flash_offset;
};

struct cs40l26_data {
	const struct device *const dev;
	struct k_mutex lock;
	uint8_t rev_id;
	struct gpio_callback interrupt_callback;
	struct k_work_delayable interrupt_worker;
	haptics_error_callback_t error_callback;
	void *user_data;
	struct k_sem calibration_semaphore;
	struct cs40l26_calibration calibration;
	uint32_t output;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L26_H_ */
