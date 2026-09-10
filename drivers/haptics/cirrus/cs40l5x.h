/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_
#define ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_

#include "cs40lxx.h"
#include <zephyr/drivers/haptics/cs40l5x.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Supported devices */
#define CS40L5X_DEVID_50  0x40A50U
#define CS40L5X_DEVID_51  0x40A51U
#define CS40L5X_DEVID_52  0x40A52U
#define CS40L5X_DEVID_53  0x40A53U
#define CS40L5X_REVID_B0  0xB0U
#define CS40L5X_OTPID_MIN 0x1U
#define CS40L5X_OTPID_MAX 0xEU

enum { /* Non-deterministic firmware registers */
	CS40L5X_REG_HALO_STATE,
	CS40L5X_REG_BUZZ_FREQ,
	CS40L5X_REG_BUZZ_LEVEL,
	CS40L5X_REG_BUZZ_DURATION,
	CS40L5X_REG_BUZZ_RES,
	CS40L5X_REG_DYNAMIC_F0,
	CS40L5X_REG_REDC,
	CS40L5X_REG_F0_EST,
	CS40L5X_REG_SOURCE_ATTENUATION,
	CS40L5X_REG_LOGGER_ENABLE,
	CS40L5X_REG_LOGGER_DATA,
	CS40L5X_REG_GPIO_EVENT_BASE,
	CS40L5X_REG_STDBY_TIMEOUT,
	CS40L5X_REG_ACTIVE_TIMEOUT,
	CS40L5X_REG_MAILBOX_QUEUE_WT,
	CS40L5X_REG_MAILBOX_QUEUE_RD,
	CS40L5X_REG_MAILBOX_STATUS,
	CS40L5X_REG_WSEQ_POWER,
	CS40L5X_REG_VIBEGEN_F0_OTP_STORED,
	CS40L5X_REG_VIBEGEN_REDC_OTP_STORED,
	CS40L5X_REG_VIBEGEN_COMPENSATION_ENABLE,
	CS40L5X_REG_RTH_HEADER1_0,
	CS40L5X_REG_RTH_HEADER2_0,
	CS40L5X_REG_RTH_DATA_0,
	CS40L5X_REG_RTH_HEADER1_1,
	CS40L5X_REG_RTH_HEADER2_1,
	CS40L5X_REG_RTH_DATA_1,
	CS40L5X_REG_CALIB_REDC_EST,
	CS40L5X_NUM_FIRMWARE_REGISTERS,
};

struct cs40l5x_calibration {
	/* Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/* Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

struct cs40l5x_sensor {
	bool is_signed;
	uint8_t n;
	uint8_t m;
	int8_t full_scale;
};

struct cs40l5x_config {
	LOG_INSTANCE_PTR_DECLARE(log);
	/* Log instance declaration requires blank line. */
	const struct device *const dev;
	const uint32_t dev_id;
	const struct cs40lxx_io_bus io_bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec interrupt_gpio;
	const struct gpio_dt_spec *const trigger_gpios;
	const int *const trigger_mapping;
	const uint8_t num_triggers;
	const struct device *flash;
	const off_t flash_offset;
	const struct device *const external_boost;
};

struct cs40l5x_data {
	const struct device *const dev;
	struct k_mutex lock;
	uint8_t rev_id;
	struct gpio_callback interrupt_callback;
	struct k_work_delayable interrupt_worker;
	haptics_error_callback_t error_callback;
	void *user_data;
	struct k_sem calibration_semaphore;
	struct cs40l5x_calibration calibration;
	uint32_t output;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_ */
