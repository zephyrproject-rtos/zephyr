/*
 * Copyright 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup haptics_interface
 * @brief Main header file for haptics driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_

/**
 * @brief Interfaces for haptic drivers.
 * @defgroup haptics_interface Haptics
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup haptics_interface_ext Device-specific Haptics API extensions
 *
 * @{
 * @}
 */

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Haptics error types
 *
 * Most haptic drivers provide protection features to prevent damage during operation.
 * Applications can register a callback function (see @ref haptics_register_error_callback()) to
 * handle or recover from error states. The following error types are used to construct a bitmask
 * provided to the application layer at run-time (see @ref haptics_error_callback_t).
 */
enum haptics_error_type {
	HAPTICS_ERROR_OVERCURRENT = BIT(0),     /**< Output overcurrent error */
	HAPTICS_ERROR_OVERTEMPERATURE = BIT(1), /**< Device overtemperature error */
	HAPTICS_ERROR_UNDERVOLTAGE = BIT(2),    /**< Power source undervoltage error */
	HAPTICS_ERROR_OVERVOLTAGE = BIT(3),     /**< Power source overvoltage error */
	HAPTICS_ERROR_DC = BIT(4),              /**< Output direct-current error */

	/* Device-specific error codes can follow, refer to the device’s header file */
	/** @cond INTERNAL_HIDDEN */
	HAPTICS_ERROR_PRIV_START = BIT(5),
	/** @endcond */
};

/**
 * @brief Integrated sensing features
 *
 * Some haptic drivers include integrated sensors for monitoring electrical and physical
 * characteristics (e.g., voltage, current, resistance, etc.) related to power or an external
 * actuator. Use any of the following monitor options with @ref haptics_monitor_get() or
 * @ref haptics_monitor_set() to retrieve sensor readings or enable measurements, respectively.
 */
enum haptics_monitor {
	HAPTICS_MONITOR_BEMF,         /**< Back EMF */
	HAPTICS_MONITOR_CURRENT,      /**< Amperage */
	HAPTICS_MONITOR_F0,           /**< Resonant frequency */
	HAPTICS_MONITOR_RE,           /**< Coil resistance */
	HAPTICS_MONITOR_AMBIENT_TEMP, /**< Ambient temperature */
	HAPTICS_MONITOR_DIE_TEMP,     /**< Die temperature */
	HAPTICS_MONITOR_VBAT,         /**< Supply voltage */
	HAPTICS_MONITOR_VBST,         /**< Boost supply voltage */
	HAPTICS_MONITOR_VOUT,         /**< Amplifier output voltage */
	HAPTICS_MONITOR_ALL,          /**< All integrated sensors */

	/* Device-specific sensing options, refer to the device's header file */
	/** @cond INTERNAL_HIDDEN */
	HAPTICS_MONITOR_PRIV_START,
	/** @endcond */
};

/**
 * @brief Qualifiers for integrated sensor readings
 *
 * There is no uniform implementation for integrated sensors in haptic drivers. Some only provide
 * the most recent sensor reading, and others aggregate readings over time into minimums, maximums,
 * and averages. Refer to device drivers for implementation details.
 */
enum haptics_monitor_type {
	HAPTICS_MONITOR_TYPE_MIN,    /**< Minimum sensor reading */
	HAPTICS_MONITOR_TYPE_MAX,    /**< Maximum sensor reading */
	HAPTICS_MONITOR_TYPE_MEAN,   /**< Mean sensor reading */
	HAPTICS_MONITOR_TYPE_SINGLE, /**< Most recent or single-shot sensor reading */

	/* Device-specific monitor sample types, refer to the device's header file */
	/** @cond INTERNAL_HIDDEN */
	HAPTICS_MONITOR_TYPE_PRIV_START,
	/** @endcond */
};

/**
 * @brief Haptics playback sources
 *
 * Haptic effects are typically pre-programmed in non-volatile memory, programmed at run-time into
 * volatile memory, or streamed to the haptic driver over a control port or using a well-defined
 * digital interface (e.g., I2S) or analog signal. Provide any of the following playback sources to
 * @ref haptics_select_source() to configure the device before calling @ref haptics_start_output().
 */
enum haptics_source {
	HAPTICS_SOURCE_ROM,          /**< Effect from a pre-programmed wavetable */
	HAPTICS_SOURCE_RAM,          /**< Effect loaded at runtime into volatile memory */
	HAPTICS_SOURCE_DAI,          /**< Effect streamed from a digital audio source (e.g., I2S) */
	HAPTICS_SOURCE_ANALOG,       /**< Effect streamed from an analog source */
	HAPTICS_SOURCE_PWM,          /**< Effect streamed from a PWM source */
	HAPTICS_SOURCE_CONTROL_PORT, /**< Effect streamed via the control port (e.g., I2C or SPI) */
	HAPTICS_SOURCE_ALL,          /**< All playback sources */

	/* Device-specific playback sources, refer to the device’s header file */
	/** @cond INTERNAL_HIDDEN */
	HAPTICS_SOURCE_PRIV_START,
	/** @endcond */
};

/**
 * @brief Haptics source configuration
 *
 * Most haptic drivers provide pre-programmed effects or expose methods for programming new effects
 * at run-time. Such effects are typically stored in what is called a wavetable, where individual
 * effects can be referenced using an index. To specify a particular effect from a wavetable, use
 * @p idx with @ref haptics_select_source(). Otherwise, @p dai is provided to specify bus
 * configuration details for digital audio interfaces, notably I2S and PCM.
 */
union haptics_config {
	uint32_t idx; /**< Used to specify an effect from a list of waveforms stored in memory */
	struct {
		audio_dai_type_t type; /**< Digital audio interface type, e.g., I2S */
		audio_dai_cfg_t cfg;   /**< Digital audio interface configuration details */
	} dai; /**< Digital audio interface configuration for "audio to haptics" features */
};

/**
 * @brief Function type of callback invoked when an error occurs.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] errors Device errors (bitmask of @ref haptics_error_type values)
 * @param[in] user_data User data provided when the error callback was registered
 */
typedef void (*haptics_error_callback_t)(const struct device *dev, const uint32_t errors,
					 void *const user_data);

/**
 * @def_driverbackendgroup{Haptics,haptics_interface}
 * @{
 */

/**
 * @brief Calibrate the haptic driver for an external actuator.
 * See haptics_calibrate() for argument description.
 */
typedef int (*haptics_calibrate_t)(const struct device *dev, const uint32_t routine);

/**
 * @brief Retrieve an integrated sensor reading from the haptic driver.
 * See haptics_monitor_get() for argument description.
 */
typedef int (*haptics_monitor_get_t)(const struct device *dev, const enum haptics_monitor monitor,
				     const enum haptics_monitor_type type,
				     struct sensor_value *const val);

/**
 * @brief Enable or disable integrated sensing for the haptic driver.
 * See haptics_monitor_set() for argument description.
 */
typedef int (*haptics_monitor_set_t)(const struct device *dev, const enum haptics_monitor monitor,
				     const bool enable);

/**
 * @brief Register a callback function for haptics errors.
 * See haptics_register_error_callback() for argument description.
 */
typedef int (*haptics_register_error_callback_t)(const struct device *dev,
						 haptics_error_callback_t cb,
						 void *const user_data);

/**
 * @brief Specify a playback source for @ref haptics_start_output().
 * See haptics_select_source() for argument description.
 */
typedef int (*haptics_select_source_t)(const struct device *dev, const enum haptics_source src,
				       const union haptics_config *const cfg);

/**
 * @brief Set level controls for haptic effects.
 * See haptics_set_level() for argument description.
 */
typedef int (*haptics_set_level_t)(const struct device *dev, const enum haptics_source src,
				   const union haptics_config *const cfg, const uint32_t level);

/**
 * @brief Start playback for a haptic effect.
 * See haptics_start_output() for argument description.
 */
typedef int (*haptics_start_output_t)(const struct device *dev);

/**
 * @brief Stop an ongoing haptic effect and cancel any queued effects, if applicable.
 * See haptics_stop_output() for argument description.
 */
typedef int (*haptics_stop_output_t)(const struct device *dev);

/**
 * @brief Stream 8-bit samples over the control port for haptic effects.
 * See haptics_stream_samples() for argument description.
 */
typedef int (*haptics_stream_samples_t)(const struct device *dev, const uint8_t *const samples,
					const size_t len);

/**
 * @driver_ops{Haptics}
 */
__subsystem struct haptics_driver_api {
	/**
	 * @driver_ops_optional @copybrief haptics_calibrate
	 */
	haptics_calibrate_t calibrate;
	/**
	 * @driver_ops_optional @copybrief haptics_monitor_get
	 */
	haptics_monitor_get_t monitor_get;
	/**
	 * @driver_ops_optional @copybrief haptics_monitor_set
	 */
	haptics_monitor_set_t monitor_set;
	/**
	 * @driver_ops_optional @copybrief haptics_register_error_callback
	 */
	haptics_register_error_callback_t register_error_callback;
	/**
	 * @driver_ops_mandatory @copybrief haptics_select_source
	 */
	haptics_select_source_t select_source;
	/**
	 * @driver_ops_optional @copybrief haptics_set_level
	 */
	haptics_set_level_t set_level;
	/**
	 * @driver_ops_mandatory @copybrief haptics_start_output
	 */
	haptics_start_output_t start_output;
	/**
	 * @driver_ops_mandatory @copybrief haptics_stop_output
	 */
	haptics_stop_output_t stop_output;
	/**
	 * @driver_ops_optional @copybrief haptics_stream_samples
	 */
	haptics_stream_samples_t stream_samples;
};

/**
 * @}
 */

/**
 * @brief Calibrate the haptic driver for an external actuator
 *
 * Most haptic drivers provide calibration features that tune the hardware for an external
 * actuator's particular physical and electrical characteristics, e.g., coil resistance and
 * resonant frequency. Not all haptic drivers require discrete calibration routines. Refer to
 * device drivers for implementation details.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] routine Device-specific calibration routine, refer to the device's header file
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
__syscall int haptics_calibrate(const struct device *dev, const uint32_t routine);

static inline int z_impl_haptics_calibrate(const struct device *dev, const uint32_t routine)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->calibrate == NULL) {
		return -ENOSYS;
	}

	return api->calibrate(dev, routine);
}

/**
 * @brief Retrieve an integrated sensor reading from the haptic driver
 *
 * Some haptic drivers may require enabling an integrated sensor using @ref haptics_monitor_set()
 * before values can be read from the device. Refer to device drivers for implementation details.
 *
 * Note: Requesting readings from all sensors simultaneously (e.g., setting @p monitor to
 * HAPTICS_MONITOR_ALL) is invalid.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] monitor Sensing option (of type @ref haptics_monitor)
 * @param[in] type Type of sensor reading (of type @ref haptics_monitor_type)
 * @param[out] val Sensor reading (of type @ref sensor_value)
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
__syscall int haptics_monitor_get(const struct device *dev, const enum haptics_monitor monitor,
				  const enum haptics_monitor_type type,
				  struct sensor_value *const val);

static inline int z_impl_haptics_monitor_get(const struct device *dev,
					     const enum haptics_monitor monitor,
					     const enum haptics_monitor_type type,
					     struct sensor_value *const val)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->monitor_get == NULL) {
		return -ENOSYS;
	}

	__ASSERT_NO_MSG(monitor != HAPTICS_MONITOR_ALL);
	__ASSERT_NO_MSG(val != NULL);

	return api->monitor_get(dev, monitor, type, val);
}

/**
 * @brief Enable or disable integrated sensing for the haptic driver
 *
 * Some haptic drivers may allow disabling integrated sensor measurements to reduce power
 * consumption. Refer to device drivers for implementation details.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] monitor Sensing option (of type @ref haptics_monitor)
 * @param[in] enable True if enabling integrated sensing, false if disabling
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
__syscall int haptics_monitor_set(const struct device *dev, const enum haptics_monitor monitor,
				  const bool enable);

static inline int z_impl_haptics_monitor_set(const struct device *dev,
					     const enum haptics_monitor monitor, const bool enable)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->monitor_set == NULL) {
		return -ENOSYS;
	}

	return api->monitor_set(dev, monitor, enable);
}

/**
 * @brief Register a callback function for haptics errors
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] cb Callback function (of type @ref haptics_error_callback_t)
 * @param[in] user_data User data to be provided back to the application via the callback
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
static inline int haptics_register_error_callback(const struct device *dev,
						  haptics_error_callback_t cb,
						  void *const user_data)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->register_error_callback == NULL) {
		return -ENOSYS;
	}

	__ASSERT_NO_MSG(cb != NULL);

	return api->register_error_callback(dev, cb, user_data);
}

/**
 * @brief Specify a playback source and related configuration details
 *
 * Typically, an application should call this function before calling @ref haptics_start_output().
 * If unused, there is no uniform default playback source across haptic drivers, so behavior is up
 * to the particular device driver. Refer to device drivers for implementation details.
 *
 * Note: Some playback sources do not require any additional configuration details, in which case
 * @p cfg should be set to NULL.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] src Playback source (of type @ref haptics_source)
 * @param[in] cfg Source configuration (of type @ref haptics_config) or NULL
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
__syscall int haptics_select_source(const struct device *dev, const enum haptics_source src,
				    const union haptics_config *const cfg);

static inline int z_impl_haptics_select_source(const struct device *dev,
					       const enum haptics_source src,
					       const union haptics_config *const cfg)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	__ASSERT_NO_MSG(api->select_source != NULL);

	return api->select_source(dev, src, cfg);
}

/**
 * @brief Set level controls for haptic effects
 *
 * Most haptic drivers support configuring output level on a global, source-, or even
 * waveform-specific basis. For example, to set global level for a haptic driver, an application
 * might set @p src to HAPTICS_SOURCE_ALL and @p cfg to NULL, while level configuration for a
 * pre-programmed waveform might set @p src to HAPTICS_SOURCE_ROM and @p cfg to the desired 'idx'
 * value. Refer to device drivers for implementation details.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] src Playback source (of type @ref haptics_source)
 * @param[in] cfg Source configuration (of type @ref haptics_config) or NULL
 * @param[in] level Device-specific output level value, rfer to the device driver for details
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
__syscall int haptics_set_level(const struct device *dev, const enum haptics_source src,
				const union haptics_config *const cfg, const uint32_t level);

static inline int z_impl_haptics_set_level(const struct device *dev, const enum haptics_source src,
					   const union haptics_config *const cfg,
					   const uint32_t level)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->set_level == NULL) {
		return -ENOSYS;
	}

	return api->set_level(dev, src, cfg, level);
}

/**
 * @brief Start playback for a haptic effect
 *
 * Start playback for a haptic effect specified using @ref haptics_select_source(). If no source is
 * specified, behavior is driver-specific. Refer to device drivers for implementation details.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
__syscall int haptics_start_output(const struct device *dev);

static inline int z_impl_haptics_start_output(const struct device *dev)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	__ASSERT_NO_MSG(api->start_output != NULL);

	return api->start_output(dev);
}

/**
 * @brief Stop an ongoing haptic effect and cancel any queued effects, if applicable
 *
 * If desired, any queued effects must be re-queued using @ref haptics_start_output().
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
__syscall int haptics_stop_output(const struct device *dev);

static inline int z_impl_haptics_stop_output(const struct device *dev)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	__ASSERT_NO_MSG(api->stop_output != NULL);

	return api->stop_output(dev);
}

/**
 * @brief Stream 8-bit samples over the control port for haptic effects
 *
 * Most haptic drivers play samples streamed over a control port at a fixed sample rate. Refer to
 * device drivers for implementation details.
 *
 * Note: HAPTICS_SOURCE_CONTROL_PORT should be provided to @ref haptics_select_source() before
 * calling this function.
 *
 * @param[in] dev Pointer to the device structure for the haptic device instance
 * @param[in] samples Pointer to an array of 8-bit samples
 * @param[in] len Number of samples to write
 *
 * @retval 0 if successful
 * @retval -ENOSYS if not implemented
 * @retval <0 if failed
 */
__syscall int haptics_stream_samples(const struct device *dev, const uint8_t *const samples,
				     const size_t len);

static inline int z_impl_haptics_stream_samples(const struct device *dev,
						const uint8_t *const samples, const size_t len)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->stream_samples == NULL) {
		return -ENOSYS;
	}

	__ASSERT_NO_MSG(samples != NULL);

	return api->stream_samples(dev, samples, len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/haptics.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_ */
