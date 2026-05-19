/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_
#define ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_

#include <zephyr/drivers/haptics/cs40l5x.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CS40L5X_REG_WIDTH  4
#define CS40L5X_ADDR_WIDTH 4

struct cs40l5x_calibration {
	/* Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/* Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

struct cs40l5x_trigger_gpios {
	struct gpio_dt_spec *gpio;
	const uint8_t num_gpio;
	bool *ready;
	/* Addresses for rising- and falling-edge events */
	uint8_t *rising_edge;
	uint8_t *falling_edge;
};

typedef bool (*cs40l5x_io_bus_is_ready)(const struct device *const dev);
typedef const struct device *const (*cs40l5x_io_bus_get_device)(const struct device *const dev);
typedef int (*cs40l5x_io_bus_read)(const struct device *const dev, const uint32_t addr,
				   uint32_t *const rx, const uint32_t len);
typedef int (*cs40l5x_io_bus_write)(const struct device *const dev, const uint32_t addr,
				    uint32_t *const tx, const uint32_t len);
typedef int (*cs40l5x_io_bus_raw_write)(const struct device *const dev, const uint32_t addr,
					uint32_t *const tx, const uint32_t len);

struct cs40l5x_bus_io {
	cs40l5x_io_bus_is_ready is_ready;
	cs40l5x_io_bus_get_device get_device;
	cs40l5x_io_bus_read read;
	cs40l5x_io_bus_write write;
	cs40l5x_io_bus_raw_write raw_write;
};

#if CONFIG_HAPTICS_CS40L5X_I2C
extern const struct cs40l5x_bus_io cs40l5x_bus_io_i2c;
#endif /* CONFIG_HAPTICS_CS40L5X_I2C */

#if CONFIG_HAPTICS_CS40L5X_SPI
extern const struct cs40l5x_bus_io cs40l5x_bus_io_spi;
#endif /* CONFIG_HAPTICS_CS40L5X_SPI */

struct cs40l5x_bus {
#if CONFIG_HAPTICS_CS40L5X_I2C
	struct i2c_dt_spec i2c;
#endif /* CONFIG_HAPTICS_CS40L5X_I2C */
#if CONFIG_HAPTICS_CS40L5X_SPI
	struct spi_dt_spec spi;
#endif /* CONFIG_HAPTICS_CS40L5X_SPI */
};

struct cs40l5x_config {
	LOG_INSTANCE_PTR_DECLARE(log);
	/* Log instance declaration requires blank line. */
	const struct device *const dev;
	const uint32_t dev_id;
	const struct cs40l5x_bus bus;
	const struct cs40l5x_bus_io *const bus_io;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec interrupt_gpio;
	struct cs40l5x_trigger_gpios trigger_gpios;
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
	bool custom_effects[CS40L5X_NUM_CUSTOM_EFFECTS];
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_ */
