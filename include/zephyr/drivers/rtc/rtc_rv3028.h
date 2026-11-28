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

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <stdint.h>

/**
 * @brief Handle an RV3028 timestamp event.
 *
 * Callback invoked from system workqueue context when a timestamp event occurs.
 *
 * @param dev Pointer to the RTC device that captured the event.
 * @param user_data Application-defined context supplied when registering the callback.
 */
typedef void (*rtc_rv3028_timestamp_callback)(const struct device *dev, void *user_data);

/**
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Get the stored RV3028 timestamp.
 *
 * The timestamp is captured from the external event input or the backup-power
 * switchover source selected in devicetree. Reading it clears the pending event
 * flag. Note that when configured with timestamp-overwrite "first", the
 * hardware retains the first timestamp until rtc_rv3028_timestamp_reset() is
 * called to re-arm capture.
 *
 * @param dev Pointer to the RTC device.
 * @param[out] count Pointer to the event counter associated with the captured
 *                    timestamp, or NULL if not needed.
 * @param[out] timeptr Pointer to the structure that receives the captured time.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p timeptr is NULL.
 * @retval -ENODATA No event captured (hardware event counter is 0).
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_get_timestamp(const struct device *dev, uint16_t *count,
				       struct rtc_time *timeptr);

/**
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Set a callback for RV3028 timestamp events.
 *
 * Timestamp capture must be enabled using rtc_rv3028_timestamp_enable()
 * for events to be captured and reported. Pass NULL for @p callback to disable
 * event notifications. The callback and @p user_data must remain valid until
 * the callback is replaced or disabled.
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
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Check whether an RV3028 timestamp event is pending.
 *
 * This function does not clear the event flag.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 No timestamp event is pending.
 * @retval 1 A timestamp event is pending.
 * @retval -ENOTSUP Pending status polling is not supported when interrupt-based
 *                     timestamp handling is enabled
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_is_pending(const struct device *dev);

/**
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Enable timestamp capture.
 *
 * Calling this function resets any previously captured timestamp data and
 * event counter before enabling capture.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_enable(const struct device *dev);

/**
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Disable timestamp capture.
 *
 * @param dev Pointer to the RTC device.
 *
 * Calling this function also resets any previously captured timestamp data and
 * event counter.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_disable(const struct device *dev);

/**
 * @kconfig_dep{CONFIG_RTC_RV3028_TIMESTAMP}
 *
 * @brief Reset timestamp info.
 *
 * @param dev Pointer to the RTC device.
 *
 * @retval 0 Success.
 * @retval <0 Error communicating with the device.
 */
int rtc_rv3028_timestamp_reset(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_RTC_RTC_RV3028_H */
