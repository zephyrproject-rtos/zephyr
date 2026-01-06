/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L5X_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L5X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_instance.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs40l5x_interface CS40L5x
 * @ingroup haptics_interface_ext
 * @brief CS40L5x Haptic Driver
 * @{
 */

/**
 * @brief Attenuation options for GPIO-triggered haptic effects
 *
 * @details Provide to @ref cs40l5x_configure_trigger().
 */
enum cs40l5x_attenuation {
	CS40L5X_ATTENUATION_7DB = -7, /**< Configure haptic effect with 7 dB attenuation */
	CS40L5X_ATTENUATION_6DB,      /**< Configure haptic effect with 6 dB attenuation */
	CS40L5X_ATTENUATION_5DB,      /**< Configure haptic effect with 5 dB attenuation */
	CS40L5X_ATTENUATION_4DB,      /**< Configure haptic effect with 4 dB attenuation */
	CS40L5X_ATTENUATION_3DB,      /**< Configure haptic effect with 3 dB attenuation */
	CS40L5X_ATTENUATION_2DB,      /**< Configure haptic effect with 2 dB attenuation */
	CS40L5X_ATTENUATION_1DB,      /**< Configure haptic effect with 1 dB attenuation */
	CS40L5X_ATTENUATION_0DB,      /**< Configure haptic effect with no attenuation */
};

/**
 * @brief Wavetable sources for haptic effects
 *
 * @details Provide to @ref cs40l5x_configure_trigger() or @ref cs40l5x_select_output().
 */
enum cs40l5x_bank {
	CS40L5X_ROM_BANK,    /**< Playback from the pre-programmed ROM library */
	CS40L5X_CUSTOM_BANK, /**< Playback from custom haptics source programmed at runtime */
	CS40L5X_BUZ_BANK,    /**< Playback from buzz source programmed at runtime */
	CS40L5X_NO_BANK,     /**< Reserved for driver error handling */
};

/**
 * @brief Calibration data for click compensation
 *
 * @details Reserved for internal driver use.
 */
struct cs40l5x_calibration {
	/**< Coil DC resistance in signed Q6.17 format (Ohms * (24 / 2.9)) */
	uint32_t redc;
	/**< Resonant frequency in signed Q9.14 format (Hz) */
	uint32_t f0;
};

/**
 * @brief Custom haptics source indices (0 or 1)
 *
 * @details Provide to @ref cs40l5x_upload_pcm() or @ref cs40l5x_upload_pwle() to specify
 * index to upload custom haptics source.
 */
enum cs40l5x_custom_index {
	CS40L5X_CUSTOM_0,           /**< Custom haptics source at index 0 */
	CS40L5X_CUSTOM_1,           /**< Custom haptics source at index 1 */
	CS40L5X_NUM_CUSTOM_EFFECTS, /**< Maximum number of custom haptics effects */
};

/**
 * @brief Options for runtime haptics logging
 *
 * @details Provide to @ref cs40l5x_logger() to update runtime haptics logging.
 */
enum cs40l5x_logger {
	CS40L5X_LOGGER_DISABLE,   /**< Disable runtime logging for the device */
	CS40L5X_LOGGER_ENABLE,    /**< Enable runtime logging for the device */
	CS40L5X_LOGGER_NO_CHANGE, /**< Use to retrieve haptics logging status without updating */
};

/**
 * @brief Options for runtime haptics logging sources
 *
 * @details Provide to @ref cs40l5x_logger_get() to get runtime device characteristics.
 */
enum cs40l5x_logger_source {
	CS40L5X_LOGGER_BEMF, /**< Back EMF (SVC) in signed Q0.23 format (full scale is 24 V) */
	CS40L5X_LOGGER_VBST, /**< Boost voltage in unsigned Q0.24 format (full scale is 14 V) */
	CS40L5X_LOGGER_VMON, /**< Output voltage in signed Q0.23 format (full scale is 24 V) */
};

/**
 * @brief Type specification for runtime haptics logging sources
 *
 * @details Provide to @ref cs40l5x_logger_get() to get minimum, mean, or maximum logged values.
 */
enum cs40l5x_logger_source_type {
	CS40L5X_LOGGER_MIN,  /**< Minimum value sampled for a logger source */
	CS40L5X_LOGGER_MAX,  /**< Maximum value sampled for a logger source */
	CS40L5X_LOGGER_MEAN, /**< Mean value sampled for a logger source */
};

/**
 * @brief PWLE section definition
 *
 * @details Provide array of sections to @ref cs40l5x_upload_pwle().
 */
struct cs40l5x_pwle_section {
	/**< Section duration in unsigned Q14.2 format (time) or Q16.0 format (half-cycles) */
	uint16_t duration;
	/**< Section frequency in unsigned Q10.2 format */
	uint16_t frequency;
	/**< Section level in unsigned Q0.11 format */
	uint16_t level;
	/**< Section flags in unsigned Q4.0 format */
	uint8_t flags;
};

/**
 * @brief Options for edge-triggered haptics effects
 *
 * @details Provide to @ref cs40l5x_configure_trigger() to specify the edge for haptic effects.
 */
enum cs40l5x_trigger_edge {
	CS40L5X_RISING_EDGE,  /**< Configure a rising-edge haptic effect */
	CS40L5X_FALLING_EDGE, /**< Configure a falling-edge haptic effect */
};

/**
 * @brief Structure to store GPIO trigger configurations
 */
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

/**
 * @brief Structure to store and handle trigger GPIOs
 */
struct cs40l5x_trigger_gpios {
	/**< Array of GPIOs provided via devicetree property 'trigger-gpios' */
	struct gpio_dt_spec *gpio;
	/**< Number of GPIOs provided via devicetree property 'trigger-gpios' */
	const uint8_t num_gpio;
	/**< Trigger configurations for rising-edge events */
	struct cs40l5x_trigger_config *rising_edge;
	/**< Trigger configurations for falling-edge events */
	struct cs40l5x_trigger_config *falling_edge;
	/**< GPIO statuses */
	bool *ready;
};

/**
 * @brief Return pointer to control port instance
 *
 * @param dev Pointer to the device structure for the haptic device instance
 *
 * @return Returns a pointer to the control port device structure
 */
typedef struct device *(*cs40l5x_io_bus_get_device)(const struct device *const dev);

/**
 * @brief Function wrapper for @ref i2c_is_ready_dt() or @ref spi_is_ready_dt()
 *
 * @param dev Pointer to the device structure for the haptic device instance
 *
 * @retval true control port is ready for use
 * @retval false control port is not ready for use
 */
typedef bool (*cs40l5x_io_bus_is_ready)(const struct device *const dev);

/**
 * @brief Function wrapper for @ref i2c_write_read_dt() or @ref spi_read_dt()
 *
 * @param dev Pointer to the device structure for the haptic device instance
 * @param addr Starting register address
 * @param rx Pointer to unsigned 32-bit storage (value or array) to store read values
 * @param len Number of registers to read
 *
 * @return a value from @ref i2c_write_read_dt() or @ref spi_read_dt()
 */
typedef int (*cs40l5x_io_bus_read)(const struct device *const dev, const uint32_t addr,
				   uint32_t *const rx, const uint32_t len);

/**
 * @brief Function wrapper for @ref i2c_write_dt() or @ref spi_read_dt()
 *
 * @param dev Pointer to the device structure for the haptic device instance
 * @param tx Unsigned 32-bit array with the base register address followed by values to write
 * @param len Pointer to unsigned 32-bit storage (value or array) to store read values
 *
 * @return a value from @ref i2c_write_dt() or @ref spi_write_dt()
 */
typedef int (*cs40l5x_io_bus_write)(const struct device *const dev, uint32_t *const tx,
				    const uint32_t len);

/**
 * @brief Control port I/O functions
 */
struct cs40l5x_bus_io {
	/**< Get control device instance for PM runtime usage */
	cs40l5x_io_bus_get_device get_device;
	/**< Check if control port device is ready */
	cs40l5x_io_bus_is_ready is_ready;
	/**< Read from the device */
	cs40l5x_io_bus_read read;
	/**< Write to the device  */
	cs40l5x_io_bus_write write;
};

#if CONFIG_HAPTICS_CS40L5X_I2C
/**
 * @brief Expose control port I/O functions for CS40L5x I2C-based driver
 */
extern const struct cs40l5x_bus_io cs40l5x_bus_io_i2c;
#endif /* CONFIG_HAPTICS_CS40L5X_I2C */

#if CONFIG_HAPTICS_CS40L5X_SPI
/**
 * @brief Expose control port I/O functions for CS40L5x SPI-based driver
 */
extern const struct cs40l5x_bus_io cs40l5x_bus_io_spi;
#endif /* CONFIG_HAPTICS_CS40L5X_SPI */

/**
 * @brief Structure to store control port devices
 *
 * @details Note that I2C and SPI control port structures will be included if there are multiple
 * devices in the devicetree and there is at least one device on both I2C and SPI.
 */
struct cs40l5x_bus {
#if CONFIG_HAPTICS_CS40L5X_I2C
	/**< I2C-based control port */
	struct i2c_dt_spec i2c;
#endif /* CONFIG_HAPTICS_CS40L5X_I2C */
#if CONFIG_HAPTICS_CS40L5X_SPI
	/**< SPI-based control port */
	struct spi_dt_spec spi;
#endif /* CONFIG_HAPTICS_CS40L5X_SPI */
};

/**
 * @brief CS40L5x configuration structure for a device instance
 */
struct cs40l5x_config {
	/**< Pointer to CS40L5x device instance */
	const struct device *const dev;
	/**< Pointer to data structure for CS40L5x device instance */
	struct cs40l5x_data *const data;
	/**< Logger configuration for instance-based logging */
	LOG_INSTANCE_PTR_DECLARE(log);
	/**< Control port devices */
	const struct cs40l5x_bus bus;
	/**< Control port I/O functions */
	const struct cs40l5x_bus_io *const bus_io;
	/**< Required GPIO for hardware resets */
	struct gpio_dt_spec reset_gpio;
	/**< Optional GPIO for hardware and DSP interrupts */
	struct gpio_dt_spec interrupt_gpio;
	/**< Optional GPIOs for edge-triggered haptic effects */
	struct cs40l5x_trigger_gpios trigger_gpios;
	/**< Optional flash storage, configurable via devicetree property 'flash-storage' */
	const struct device *flash;
	/**< Optional flash register offset, configurable via devicetree property 'flash-offset' */
	const off_t flash_offset;
	/**< Boost setting, configurable via devicetree property 'external-boost' */
	const struct device *const external_boost;
};

/**
 * @brief CS40L5x data structure for a device instance
 */
struct cs40l5x_data {
	/**< Pointer to CS40L5x device instance */
	const struct device *const dev;
	/**< Pointer to configuration structure for CS40L5x device instance */
	const struct cs40l5x_config *const config;
	/**< Lock to deconflict haptic playback, device calibration, and configuration changes */
	struct k_mutex lock;
	/**< Device ID corresponding to the part number */
	uint32_t dev_id;
	/**< Revision ID corresponding to the silicon variant */
	uint8_t rev_id;
	/**< Callback handler for interrupt processing */
	struct gpio_callback interrupt_callback;
	/**< Worker for debounced interrupt processing */
	struct k_work_delayable interrupt_worker;
	/**< Callback handler for trigger logging */
	struct gpio_callback trigger_callback;
	/**< Application-provided callback to recover from fatal hardware errors */
	haptics_error_callback_t error_callback;
	/**< Application-provided user data for callback context */
	void *user_data;
	/**< Semaphore used to sequence the calibration routine */
	struct k_sem calibration_semaphore;
	/**< F0 and ReDC data derived from calibration */
	struct cs40l5x_calibration calibration;
	/**< Playback command for mailbox-triggered haptic effects */
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

/**
 * @brief Run calibration to derive ReDC and F0 values and apply results for click compensation
 *
 * @param dev Pointer to the device structure for haptic device instance
 *
 * @retval 0 if success
 * @retval -EAGAIN if ReDC or F0 estimation times out
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_calibrate(const struct device *const dev);

/**
 * @brief Configure ROM buzz for haptic playback
 *
 * @details With large amplitudes and insufficient power (e.g., in the case of internal boost
 * configurations), it's possible to induce boost errors.
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param frequency Frequency of haptic effect in Hz (default: 0xF0)
 * @param level Amplitude of haptic effect, where UINT8_MAX is 100% (default: 0x1B)
 * @param duration Playback duration in milliseconds, where 0 is infinite duration (default: 0x32)
 *
 * @retval 0 if success
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_configure_buzz(const struct device *const dev, const uint32_t frequency,
			   const uint8_t level, const uint32_t duration);

/**
 * @brief Configure edge-triggered haptic effect
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param gpio Pointer to the device structure for the GPIO used as the trigger source
 * @param bank Wavetable source for desired haptic effect, see @ref cs40l5x_bank
 * @param index Wavetable index for desired haptic effect
 * @param attenuation Attenuation in dB for desired haptic effect, see @ref cs40l5x_attenuation
 * @param edge Specify edge (rising or falling) to trigger haptic effects
 *
 * @retval 0 if success
 * @retval -EINVAL if invalid wavetable source and index provided (e.g., index out of bounds)
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_configure_trigger(const struct device *const dev, const struct gpio_dt_spec *const gpio,
			      const enum cs40l5x_bank bank, const uint8_t index,
			      const enum cs40l5x_attenuation attenuation,
			      const enum cs40l5x_trigger_edge edge);

/**
 * @brief Update runtime haptics logging and get current status
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param logger_state See @ref cs40l5x_logger
 *
 * @retval 1 if logging is enabled
 * @retval 0 if logging is disabled
 * @retval -EINVAL if invalid wavetable source or trigger GPIO provided
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_logger(const struct device *const dev, enum cs40l5x_logger logger_state);

/**
 * @brief Get runtime haptics logging data for the specified logger source
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param source See @ref cs40l5x_logger_source
 * @param value Unsigned 32-bit integer to store the retrieved data
 * @param type See @ref cs40l5x_logger_source_type
 *
 * @retval 0 if success
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_logger_get(const struct device *const dev, enum cs40l5x_logger_source source,
		       enum cs40l5x_logger_source_type type, uint32_t *const value);

/**
 * @brief Select haptic effect triggered via @ref haptics_start_output()
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param bank Wavetable source for desired haptic effect, see @ref cs40l5x_bank
 * @param index Wavetable index for desired haptic effect
 *
 * @retval 0 if success
 * @retval -EINVAL if invalid wavetable source and index provided (e.g., index out of bounds)
 */
int cs40l5x_select_output(const struct device *const dev, const enum cs40l5x_bank bank,
			  const uint8_t index);

/**
 * @brief Configure gain for haptic effects triggered via @ref haptics_start_output()
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param gain Gain setting (valid values between 0 and 100)
 *
 * @retval 0 if success
 * @retval -EINVAL if provided gain is greater than 100%
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_set_gain(const struct device *const dev, const uint8_t gain);

/**
 * @brief Upload PCM effect to the specified index
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param index See @ref cs40l5x_custom_index
 * @param redc ReDC compensation in unsigned Q5.7 format | redc * (24 / 2.9)
 * @param f0 F0 compensation in unsigned Q9.3 format | (f0 - 50) * 8
 * @param samples Array of signed 8-bit PCM samples
 * @param num_samples Number of PCM samples
 *
 * @retval 0 if success
 * @retval -EINVAL if invalid index provided (e.g., index out of bounds)
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_upload_pcm(const struct device *const dev, const enum cs40l5x_custom_index index,
		       const uint16_t redc, const uint16_t f0, const int8_t *const samples,
		       const uint16_t num_samples);

/**
 * @brief Upload PWLE effect to the specified index
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param index See @ref cs40l5x_custom_index
 * @param sections Array of @ref cs40l5x_pwle_section
 * @param num_sections Number of PWLE sections
 *
 * @retval 0 if success
 * @retval -EINVAL if invalid index provided (e.g., index out of bounds)
 * @retval -EIO if a control port transaction fails
 * @retval -errno another error code on failure, resulting from PM action callback
 */
int cs40l5x_upload_pwle(const struct device *const dev, const enum cs40l5x_custom_index index,
			const struct cs40l5x_pwle_section *const sections,
			const uint8_t num_sections);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
