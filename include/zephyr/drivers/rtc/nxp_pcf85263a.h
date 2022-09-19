/*
 * Copyright (c) 2022 Fraunhofer AICOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_PCF8526A_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_PCF8526A_H_

#include <time.h>

/** @brief Flag indicating that the alarm must use INTA
 * pin for signaling that an alarm has been triggered.
 */
#define PCF85263A_ALARM_FLAGS_USE_INTA           BIT(0)

/** @brief Flag indicating that the alarm must use INTB
 * pin for signaling that an alarm has been triggered.
 */
#define PCF85263A_ALARM_FLAGS_USE_INTB           BIT(1)

/**
 * @brief Signature for PCF85263a alarm callbacks.
 *
 * @param dev the device from which the callback originated.
 *
 * @param id the alarm id from which the callback originated.
 *
 * @param value posix offset from nxp_pcf85263a_get_value() at the
 * time the alarm interrupt was processed.
 *
 * @param user_data user provided pointer passed to alarm callback.
 */
typedef void (*nxp_pcf8523a_alarm_callback_t)(const struct device *dev,
					 uint8_t id, uint32_t value,
					 void *user_data);

/** @brief Structure defining the alarm configuration. */
struct nxp_pcf85263a_alarm_cfg {
	/** @brief Time specification for an RTC alarm. */
	time_t time;
	/** @brief Function to be called when alarm is signalled.
	 * The callback will be invoked from the system work queue.
	 */
	nxp_pcf8523a_alarm_callback_t callback;
	/** @brief User-provided pointer passed to alarm callback. */
	void *user_data;
	/** @brief Flags controlling configuration of the alarm.
	 * At the moment, two flags are available:
	 * - PCF85263A_ALARM_FLAGS_USE_INTA
	 * - PCF85263A_ALARM_FLAGS_USE_INTB
	 * This two flags define which interrupt line must be signaled
	 * when the current time reaches the time set on alarm.
	 */
	uint8_t flags;
};

/**
 * @brief Gets date and time posix offset in seconds.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @param value pointer into which current posix offset in seconds will be stored.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction or not implemented feature (stop-watch mode).
 */
int nxp_pcf85263a_get_value(const struct device *dev, uint64_t *value);

/**
 * @brief Sets date and time.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @param value date and time posix offset in seconds.
 *
 * @note This function starts time counting right after setting a new initial value.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction or not implemented feature (stop-watch mode).
 */
int nxp_pcf85263a_set_value(const struct device *dev, uint64_t value);

/**
 * @brief Starts time counting.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction.
 */
int nxp_pcf85263a_start(const struct device *dev);

/**
 * @brief Stops time counting.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction.
 */
int nxp_pcf85263a_stop(const struct device *dev);

/**
 * @brief Sets alarm on PCF85263a RTC.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @param id the alarm id. PCF85263a supports two alarms simultaneously. @c ALARM1 is 1
 * and can be configured froms seconds to months. @c ALARM2 is 2 and operates on minutes,
 * seconds and weekday.
 *
 * @param alarm_cfg a pointer to the desired alarm configuration.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction or an invallid parameter.
 */
int nxp_pcf85263a_set_alarm(const struct device *dev, uint8_t id,
	const struct nxp_pcf85263a_alarm_cfg *alarm_cfg);

/**
 * @brief Cancels alarm on PCF85263a RTC.
 *
 * @param dev the PCF85263a device pointer.
 *
 * @param id the alarm id.
 *
 * @return a non-negative value on success, or a negative error code from
 * an I2C transaction or an invallid parameter.
 */
int nxp_pcf85263a_cancel_alarm(const struct device *dev, uint8_t id);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_PCF8526A_H_ */
