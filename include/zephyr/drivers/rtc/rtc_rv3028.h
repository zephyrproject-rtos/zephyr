/*
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RV3028 RTC driver specific API.
 * @ingroup rtc_interface_ext
 */

#ifndef ZEPHYR_INCLUDE_RTC_RTC_RV3028_H
#define ZEPHYR_INCLUDE_RTC_RTC_RV3028_H

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

/**
 * @brief Handle an RV3028 timestamp event.
 *
 * @param dev Pointer to the RTC device that captured the event.
 * @param user_data Application-defined context supplied when registering the callback.
 */
typedef void (*rtc_rv3028_timestamp_callback)(const struct device *dev, void *user_data);

/**
 * @brief Get the stored RV3028 timestamp.
 *
 * The timestamp is captured from the external event input or the backup-power
 * switchover source selected in devicetree. Reading it clears the pending event
 * flag.
 *
 * @param dev Pointer to the RTC device.
 * @param[out] count Pointer to the event counter associated with the captured
 *                    timestamp.
 * @param[out] timeptr Pointer to the structure that receives the captured time.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p timeptr is NULL.
 * @retval -ENODATA @p count is 0 - no event captured.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_get_timestamp(const struct device *dev, uint16_t *count,
				       struct rtc_time *timeptr);

/**
 * @brief Set a callback for RV3028 timestamp events.
 *
 * Pass NULL for @p callback to disable event notifications. The callback and
 * @p user_data must remain valid until the callback is replaced or disabled.
 *
 * @param dev       Pointer to the RTC device.
 * @param callback  Callback invoked when the device captures an event, or NULL
 *                     to disable notifications.
 * @param user_data Application-defined context passed to @p callback.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_set_callback(const struct device *dev,
				      rtc_rv3028_timestamp_callback callback, void *user_data);

/**
 * @brief Check whether an RV3028 timestamp event is pending.
 *
 * This function does not clear the event flag.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 No timestamp event is pending.
 * @retval 1 A timestamp event is pending.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_is_pending(const struct device *dev);

/**
 * @brief Enable timestamp capture.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_enable(const struct device *dev);

/**
 * @brief Disable timestamp capture.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_disable(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_RTC_RTC_RV3028_H */
