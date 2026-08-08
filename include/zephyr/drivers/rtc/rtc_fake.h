/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake RTC driver API for testing purposes.
 * @ingroup rtc_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_

#include <zephyr/drivers/rtc.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake RTC driver API functions.
 * @defgroup rtc_fake Fake RTC
 * @ingroup io_emulators
 * @ingroup rtc_interface
 * @{
 */

/**
 * @brief Set the time of the fake RTC.
 *
 * @see rtc_set_time
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_time, const struct device *, const struct rtc_time *);

/**
 * @brief Get the time of the fake RTC.
 *
 * @see rtc_get_time
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_time, const struct device *, struct rtc_time *);

#ifdef CONFIG_RTC_ALARM
/**
 * @brief Get the supported alarm time fields of the fake RTC.
 *
 * @see rtc_alarm_get_supported_fields
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_supported_fields, const struct device *, uint16_t,
			uint16_t *);

/**
 * @brief Set the alarm time of the fake RTC.
 *
 * @see rtc_alarm_set_time
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_time, const struct device *, uint16_t, uint16_t,
			const struct rtc_time *);

/**
 * @brief Get the alarm time of the fake RTC.
 *
 * @see rtc_alarm_get_time
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_time, const struct device *, uint16_t, uint16_t *,
			struct rtc_time *);

/**
 * @brief Test if a fake RTC alarm is pending.
 *
 * @see rtc_alarm_is_pending
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_is_pending, const struct device *, uint16_t);

/**
 * @brief Set the alarm callback of the fake RTC.
 *
 * @see rtc_alarm_set_callback
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_callback, const struct device *, uint16_t,
			rtc_alarm_callback, void *);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
/**
 * @brief Set the update callback of the fake RTC.
 *
 * @see rtc_update_set_callback
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_update_set_callback, const struct device *,
			rtc_update_callback, void *);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
/**
 * @brief Set the calibration of the fake RTC.
 *
 * @see rtc_set_calibration
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_calibration, const struct device *, int32_t);

/**
 * @brief Get the calibration of the fake RTC.
 *
 * @see rtc_get_calibration
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_calibration, const struct device *, int32_t *);
#endif /* CONFIG_RTC_CALIBRATION */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_ */
