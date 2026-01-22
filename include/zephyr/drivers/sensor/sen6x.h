/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of SEN6X sensor
 * @ingroup sen6x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SEN6X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SEN6X_H_

#include <zephyr/drivers/sensor.h>

/**
 * @defgroup sen6x_interface SEN6X
 * @ingroup sensor_interface_ext
 * @brief Sensirion SEN6X sensor
 * @{
 */

/** Fan is switched on, but 0 RPM is measured. Sticky. */
#define SEN6X_STATUS_FAN_ERROR     BIT(4)
/** Relative Humidity and Temperature Sensor Error. Sticky. */
#define SEN6X_STATUS_RHT_ERROR     BIT(6)
/** Gas sensor error. Sticky. */
#define SEN6X_STATUS_GAS_ERROR     BIT(7)
/** CO2 sensor error. Sticky. */
#define SEN6X_STATUS_CO2_2_ERROR   BIT(9)
/** Formaldehyde sensor error. Sticky. */
#define SEN6X_STATUS_HCHO_ERROR    BIT(10)
/** PM sensor error. Sticky. */
#define SEN6X_STATUS_PM_ERROR      BIT(11)
/** CO2 sensor error. Sticky. */
#define SEN6X_STATUS_CO2_1_ERROR   BIT(12)
/** Fan speed is too high or too low. Not sticky. */
#define SEN6X_STATUS_SPEED_WARNING BIT(21)

struct sen6x_temperature_offset_parameters {
	/** Constant temperature offset in °C, scaled with factor 200. */
	int16_t offset;
	/** Normalized temperature offset slope scaled with factor 10000. */
	int16_t slope;
	/** The time constant determines how fast the new slope and offset will be applied.
	 * After the specified value in seconds, 63% of the new slope and offset are applied.
	 * A time constant of zero means the new values will be applied immediately
	 * (within the next measure interval of 1 second).
	 */
	uint16_t time_constant;
	/** The temperature offset slot to be modified. */
	uint16_t slot;
};

struct sen6x_temperature_acceleration_parameters {
	/** Filter constant K scaled with factor 10. */
	uint16_t K;
	/** Filter constant P scaled with factor 10. */
	uint16_t P;
	/** Time constant T1 scaled with factor 10. */
	uint16_t T1;
	/** Time constant T2 scaled with factor 10. */
	uint16_t T2;
};

struct sen6x_algorithm_tuning_parameters {
	/** VOC index representing typical (average) conditions. */
	int16_t index_offset;
	/** Time constant to estimate the VOC algorithm offset from the history in hours.
	 * Past events will be forgotten after about twice the learning time.
	 */
	int16_t learning_time_offset_hours;
	/** Time constant to estimate the VOC algorithm gain from the history in hours.
	 * Past events will be forgotten after about twice the learning time.
	 */
	int16_t learning_time_gain_hours;
	/** Maximum duration of gating in minutes (freeze of estimator during high VOC index
	 * signal). Set to zero to disable the gating.
	 */
	int16_t gating_max_duration_minutes;
	/** Initial estimate for standard deviation.
	 * Lower value boosts events during initial learning period but may result
	 * in larger device-to-device variations.
	 */
	int16_t std_initial;
	/** Gain factor to amplify or to attenuate the VOC index output. */
	int16_t gain_factor;
};

struct sen6x_voc_algorithm_state {
	uint8_t buffer[12];
};

struct sen6x_firmware_version {
	uint8_t major;
	uint8_t minor;
};

struct sen6x_callback {
	sys_snode_t node;

	/**
	 * @brief Called when the status as reported by the sensor has changed.
	 *
	 * @note This is called from the system workqueue.
	 *
	 * @param[in] dev      The device of which the status has changed.
	 * @param[in] callback The callback that this function is called for.
	 * @param[in] status   A bitmask with the flags in SEN6X_STATUS_*. The
	 *                     format may be different from those in the sensor
	 *                     register.
	 */
	void (*status_changed)(const struct device *dev, struct sen6x_callback *callback,
			       uint32_t status);
};

void sen6x_add_callback(const struct device *dev, struct sen6x_callback *callback);
void sen6x_remove_callback(const struct device *dev, struct sen6x_callback *callback);

/**
 * @brief Starts continuous measurement.
 *
 * Starts a continuous measurement. After starting the measurement,
 * it takes some time (~1.1s) until the first measurement results are available.
 *
 * @note On SEN63C, this starts a 24s long CO2 sensor conditioning process. This
 *       driver enforces the limitations mentioned in the datasheet to prevent
 *       the sensor from setting error bits.
 *
 * @retval 0         On success
 * @retval -EAGAIN   Measurement can't be started (again), yet.
 * @retval -EALREADY Measurement is already running.
 * @retval -errno    For other failures.
 */
int sen6x_start_continuous_measurement(const struct device *dev);

/**
 * @brief Stop continuous measurement.
 *
 * Stops the measurement and returns to idle mode.
 * After sending this command, wait at least 1s before starting a new measurement.
 *
 * @retval 0         On success
 * @retval -EALREADY Measurement wasn't running.
 * @retval -errno    For other failures.
 */
int sen6x_stop_continuous_measurement(const struct device *dev);

/**
 * @brief Executes a reset on the device.
 *
 * This has the same effect as a power cycle. All volatile configuration will be
 * lost.
 *
 * @note On SEN60, this must only be called in idle mode.
 *
 * @retval 0      On success
 * @retval -EPERM Command is not allowed in the current mode.
 * @retval -errno For other failures.
 */
int sen6x_device_reset(const struct device *dev);

/**
 * @brief Gets the product name from the device.
 *
 * @note Not supported on SEN60.
 *
 * @param[in]  dev          The device to operate on.
 * @param[out] name         The buffer where the name will be stored.
 * @param[in]  max_name_len The size of the buffer behind \p name. Must be >= 48.
 *
 * @retval 0        On success
 * @retval -ENOBUFS \p name_len is too small.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -errno   For other failures.
 */
int sen6x_get_product_name(const struct device *dev, char *name, size_t max_name_len);

/**
 * @brief Gets the serial number from the device.
 *
 * @note On SEN60, this returns the hex representation of the 6 byte serial number.
 * @note On SEN60, this must only be called in idle mode.
 *
 * @param[in]  dev            The device to operate on.
 * @param[out] serial         The buffer where the serial will be stored.
 * @param[in]  max_serial_len The size of the buffer behind \p serial. Must be >= 48.
 *
 * @retval 0        On success
 * @retval -ENOBUFS \p serial_len is too small.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_get_serial_number(const struct device *dev, char *serial, size_t max_serial_len);

/**
 * @brief Gets the version information for the firmware.
 *
 * @note On SEN60, this returns 0.0, because it does not provide it's version.
 */
const struct sen6x_firmware_version *sen6x_get_firmware_version(const struct device *dev);

/**
 * @brief Set Temperature Offset Parameters.
 *
 * @note Not supported on SEN60.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @retval 0        On success
 * @retval -EINVAL  Parameters are out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -errno   For other failures.
 */
int sen6x_set_temperature_offset_parameters(
	const struct device *dev, const struct sen6x_temperature_offset_parameters *params);

/**
 * @brief Set Temperature Acceleration Parameters.
 *
 * @note Not supported on SEN60.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @retval 0        On success
 * @retval -EINVAL  Parameters are out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -errno   For other failures.
 */
int sen6x_set_temperature_acceleration_parameters(
	const struct device *dev, const struct sen6x_temperature_acceleration_parameters *params);

/**
 * @brief Set VOC Algorithm Tuning Parameters.
 *
 * @note Must only be called in idle mode.
 * @note Only supported on SEN65, SEN66 and SEN68.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @retval 0        On success
 * @retval -EINVAL  Parameters are out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_set_voc_algorithm_tuning_parameters(
	const struct device *dev, const struct sen6x_algorithm_tuning_parameters *params);

/**
 * @brief Set NOx Algorithm Tuning Parameters.
 *
 * @note Must only be called in idle mode.
 * @note Only supported on SEN65, SEN66 and SEN68.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @retval 0        On success
 * @retval -EINVAL  Parameters are out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_set_nox_algorithm_tuning_parameters(
	const struct device *dev, const struct sen6x_algorithm_tuning_parameters *params);

/**
 * @brief Set CO2 Sensor Automatic Self Calibration.
 *
 * @note Must only be called in idle mode.
 * @note Only supported on SEN63C and SEN66.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @retval 0        On success
 * @retval -EAGAIN  CO2 conditioning is currently running.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_set_co2_automatic_self_calibration(const struct device *dev, bool enabled);

/**
 * @brief Set Ambient Pressure.
 *
 * Sets the current ambient pressure used for pressure compensation in the CO2 sensor.
 *
 * @note Only supported on SEN63C and SEN66.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @param[in] dev   The device to operate on.
 * @param[in] value Ambient pressure in hPa.
 *
 * @retval 0        On success
 * @retval -EINVAL  \p value is out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -errno   For other failures.
 */
int sen6x_set_ambient_pressure(const struct device *dev, uint16_t value);

/**
 * @brief Set Sensor Altitude.
 *
 * Sets the current sensor altitude used for pressure compensation in the CO2 sensor.
 *
 * @note Must only be called in idle mode.
 * @note Only supported on SEN63C and SEN66.
 * @note This configuration is volatile, i.e. the parameters will be reverted to
 *       their default value of zero after a device reset.
 *
 * @param[in] dev   The device to operate on.
 * @param[in] value Ambient pressure in meters.
 *
 * @retval 0        On success
 * @retval -EINVAL  \p value is out of range.
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_set_sensor_altitude(const struct device *dev, uint16_t value);

/**
 * @brief Set Sensor Altitude.
 *
 * This command triggers fan cleaning. The fan is set to the maximum speed for 10 seconds and
 * then automatically stopped. Wait at least 10s after this command before starting a measurement.
 *
 * @note Must only be called in idle mode.
 * @note Not supported on SEN60.
 *
 * @retval 0        On success
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_start_fan_cleaning(const struct device *dev);

/**
 * @brief Activate SHT Heater.
 *
 * This command activates the SHT sensor heater with 200mW for 1s.
 * The heater is then automatically deactivated again.
 * On supported devices and firmware versions, The heater measurements are returned in
 * \p relative_humidity and \p temperature.
 *
 * @param[in]  dev               The device to operate on.
 * @param[out] relative_humidity Relative humidity in %.
 * @param[out] temperature       Temperature in °C.
 * @note Must only be called in idle mode.
 * @note Not supported on SEN60.
 *
 * @retval 0        On success
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_activate_sht_heater(const struct device *dev, struct sensor_q31_data *relative_humidity,
			      struct sensor_q31_data *temperature);

/**
 * @brief Get VOC Algorithm State.
 *
 * Allows backup of the VOC algorithm state to resume operation after a
 * power cycle or device reset, skipping initial learning phase.
 *
 * @note Only supported on SEN65, SEN66 and SEN68.
 *
 * @retval 0        On success
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -errno   For other failures.
 */
int sen6x_get_voc_algorithm_state(const struct device *dev,
				  struct sen6x_voc_algorithm_state *state);

/**
 * @brief Get VOC Algorithm State.
 *
 * Allows restoration of the VOC algorithm state to resume operation after a
 * power cycle or device reset, skipping initial learning phase.
 *
 * @note Only supported on SEN65, SEN66 and SEN68.
 * @note Must only be called in idle mode.
 *
 * @retval 0        On success
 * @retval -ENOTSUP Command is not supported on this device.
 * @retval -EPERM   Command is not allowed in the current mode.
 * @retval -errno   For other failures.
 */
int sen6x_set_voc_algorithm_state(const struct device *dev,
				  const struct sen6x_voc_algorithm_state *state);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SEN6X_H_ */
