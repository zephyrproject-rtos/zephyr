/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_RTC_RTC_MAX31331_H
#define ZEPHYR_INCLUDE_RTC_RTC_MAX31331_H

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

/**
 * @brief MAX31331 timestamp callback type
 *
 * @param dev        Pointer to the RTC device.
 * @param user_data  Pointer to user context.
 */
typedef void (*rtc_max31331_timestamp_callback)(const struct device *dev, void *user_data);

/**
 * @brief Read a stored timestamp from the MAX31331.
 *
 * @param dev     Pointer to the RTC device.
 * @param timeptr Pointer to struct rtc_time to store result.
 * @param index   Timestamp index (0–3).
 * @param flags   Pointer to variable receiving timestamp flags.
 *
 * @retval 0 If successful.
 * @retval negative errno code otherwise.
 */
int rtc_max31331_get_timestamps(const struct device *dev, struct rtc_time *timeptr, uint8_t index,
				uint8_t *flags);

/**
 * @brief Register a timestamp event callback.
 *
 * @param dev        Pointer to the RTC device.
 * @param cb         Callback function pointer.
 * @param user_data  Application-provided context.
 *
 * @retval 0 If successful.
 * @retval negative errno code otherwise.
 */
int rtc_max31331_set_timestamp_callback(const struct device *dev,
					rtc_max31331_timestamp_callback cb, void *user_data);

#endif /* ZEPHYR_DRIVERS_RTC_RTC_MAX31331_H */
