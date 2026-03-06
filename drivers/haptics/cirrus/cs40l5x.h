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

struct cs40l5x_calibration {
	/* Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/* Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

struct cs40l5x_trigger_config {
	/**< Wavetable source for desired haptic effect */
	enum cs40l5x_bank bank;
	/**< Wavetable index for desired haptic effect */
	uint8_t index;
	/**< Attenuation to be applied to haptic effect */
	int attenuation;
	/**< Offset register address that stores the GPIO trigger configuration */
	uint8_t address;
};

struct cs40l5x_trigger_gpios {
	struct gpio_dt_spec *gpio;
	const uint8_t num_gpio;
	/**< Trigger configurations for rising-edge events */
	struct cs40l5x_trigger_config *rising_edge;
	/**< Trigger configurations for falling-edge events */
	struct cs40l5x_trigger_config *falling_edge;
	/**< GPIO statuses */
	bool *ready;
};

typedef bool (*cs40l5x_io_bus_is_ready)(const struct device *const dev);
typedef struct device *(*cs40l5x_io_bus_get_device)(const struct device *const dev);
typedef int (*cs40l5x_io_bus_read)(const struct device *const dev, const uint32_t addr,
				   uint32_t *const rx, const uint32_t len);
typedef int (*cs40l5x_io_bus_write)(const struct device *const dev, uint32_t *const tx,
				    const uint32_t len);

struct cs40l5x_bus_io {
	cs40l5x_io_bus_is_ready is_ready;
	cs40l5x_io_bus_get_device get_device;
	cs40l5x_io_bus_read read;
	cs40l5x_io_bus_write write;
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
	/**< Callback handler for trigger logging */
	struct gpio_callback trigger_callback;
	/**< Application-provided callback to recover from fatal hardware errors */
	haptics_error_callback_t error_callback;
	void *user_data;
	struct k_sem calibration_semaphore;
	struct cs40l5x_calibration calibration;
	uint32_t output;
	/**< Ring buffer to cache mailbox playback history */
	struct ring_buf rb_mailbox_history;
	/**< Ring buffer to cache trigger playback history */
	struct ring_buf rb_trigger_history;
	/**< Ring buffer storage for cached mailbox playback history */
	uint8_t buf_mailbox_history[CONFIG_HAPTICS_CS40L5X_METADATA_CACHE_LEN];
	/**< Ring buffer storage for cached trigger playback history */
	uint8_t buf_trigger_history[CONFIG_HAPTICS_CS40L5X_METADATA_CACHE_LEN];
	/**< Upload status for custom effects at indices 0 and 1 */
	bool custom_effects[CS40L5X_NUM_CUSTOM_EFFECTS];
	/**< Number of haptic effects playing or suspended */
	int effects_in_flight;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40L5X_H_ */
