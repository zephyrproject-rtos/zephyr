/*
 * Copyright (c) 2026, Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_CS40L26_H_
#define ZEPHYR_DRIVERS_HAPTICS_CS40L26_H_

#include <sys/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_instance.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CS40L26_REG_WIDTH  4
#define CS40L26_ADDR_WIDTH 4

/* Supported devices */
#define CS40L26_DEVID_A1  0x40A260U
#define CS40L26_REVID_A1  0xA1U
#define CS40L27_DEVID_B2  0x40A270U
#define CS40L27_REVID_B2  0xB2U
#define CS40L26_OTPID_MIN 0x1U
#define CS40L26_OTPID_MAX 0xEU

/* Non-deterministic firmware registers */
#define CS40L26_REG_PM_TIMER_TIMEOUT_TICKS      0
#define CS40L26_REG_PM_ACTIVE_TIMEOUT           1
#define CS40L26_REG_PM_STDBY_TIMEOUT            2
#define CS40L26_REG_PM_POWER_ON_SEQUENCE        3
#define CS40L26_REG_DYNAMIC_F0_ENABLED          4
#define CS40L26_REG_HALO_STATE                  5
#define CS40L26_REG_RE_EST_STATUS               6
#define CS40L26_REG_VIBEGEN_F0_OTP_STORED       7
#define CS40L26_REG_VIBEGEN_REDC_OTP_STORED     8
#define CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE 9
#define CS40L26_REG_F0_EST_REDC                 10
#define CS40L26_REG_F0_EST                      11
#define CS40L26_REG_LOGGER_ENABLE               12
#define CS40L26_REG_LOGGER_DATA                 13
#define CS40L26_REG_BUZZ_FREQ                   14
#define CS40L26_REG_BUZZ_LEVEL                  15
#define CS40L26_REG_BUZZ_DURATION               16
#define CS40L26_REG_MAILBOX_QUEUE_WT            17
#define CS40L26_REG_MAILBOX_QUEUE_RD            18
#define CS40L26_REG_MAILBOX_STATUS              19
#define CS40L26_REG_SOURCE_ATTENUATION          20

struct cs40l26_multi_write {
	uint32_t addr;
	uint32_t *buf;
	size_t len;
};

struct cs40l26_calibration {
	/* Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/* Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

typedef bool (*cs40l26_io_bus_is_ready)(const struct device *const dev);
typedef const struct device *const (*cs40l26_io_bus_get_device)(const struct device *const dev);
typedef int (*cs40l26_io_bus_read)(const struct device *const dev, const uint32_t addr,
				   uint32_t *const rx, const uint32_t len);
typedef int (*cs40l26_io_bus_write)(const struct device *const dev, const uint32_t addr,
				    uint32_t *const tx, const uint32_t len);
typedef int (*cs40l26_io_bus_raw_write)(const struct device *const dev, const uint32_t addr,
					uint32_t *const tx, const uint32_t len);

struct cs40l26_bus_io {
	cs40l26_io_bus_is_ready is_ready;
	cs40l26_io_bus_get_device get_device;
	cs40l26_io_bus_read read;
	cs40l26_io_bus_write write;
	cs40l26_io_bus_raw_write raw_write;
};

#if CONFIG_HAPTICS_CS40L26_I2C
extern const struct cs40l26_bus_io cs40l26_bus_io_i2c;
#endif /* CONFIG_HAPTICS_CS40L26_I2C */

#if CONFIG_HAPTICS_CS40L26_SPI
extern const struct cs40l26_bus_io cs40l26_bus_io_spi;
#endif /* CONFIG_HAPTICS_CS40L26_SPI */

struct cs40l26_bus {
#if CONFIG_HAPTICS_CS40L26_I2C
	struct i2c_dt_spec i2c;
#endif /* CONFIG_HAPTICS_CS40L26_I2C */
#if CONFIG_HAPTICS_CS40L26_SPI
	struct spi_dt_spec spi;
#endif /* CONFIG_HAPTICS_CS40L26_SPI */
};

struct cs40l26_config {
	LOG_INSTANCE_PTR_DECLARE(log);
	/* Log instance declaration requires blank line. */
	const struct device *const dev;
	const uint32_t dev_id;
	const struct cs40l26_bus bus;
	const struct cs40l26_bus_io *const bus_io;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec interrupt_gpio;
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

int cs40l26_firmware_read(const struct device *const dev, const uint32_t firmware_control,
			  uint32_t *const rx);
int cs40l26_firmware_write(const struct device *const dev, const uint32_t firmware_control,
			   uint32_t val);
int cs40l26_firmware_burst_write(const struct device *const dev, const uint32_t firmware_control,
				 uint32_t *const tx, const uint32_t len);
int cs40l26_firmware_raw_write(const struct device *const dev, const uint32_t firmware_control,
			       uint32_t *const tx, const uint32_t len);
int cs40l26_firmware_multi_write(const struct device *const dev,
				 const struct cs40l26_multi_write *const multi_write,
				 const uint32_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CS40L26_H_ */
