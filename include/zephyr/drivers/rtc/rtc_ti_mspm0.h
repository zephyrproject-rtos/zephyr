/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup ti_mspm0_rtc_interface
 * @brief Public API for TI MSPM0 RTC driver extensions
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_TI_MSPM0_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_TI_MSPM0_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TI MSPM0 RTC device-specific API extension
 * @defgroup ti_mspm0_rtc_interface TI MSPM0 RTC
 * @since 4.5.0
 * @version 0.1.0
 * @ingroup rtc_interface
 * @{
 */

/**
 * @brief RTC interval timer event selection
 *
 * Selects the interval at which the RTCTEV interrupt is generated.
 */
enum rtc_mspm0_interval {
	/** Interrupt fires when the minute changes */
	RTC_MSPM0_INTERVAL_MINUTE   = 0,
	/** Interrupt fires when the hour changes */
	RTC_MSPM0_INTERVAL_HOUR     = 1,
	/** Interrupt fires every day at midnight (00:00) */
	RTC_MSPM0_INTERVAL_MIDNIGHT = 2,
	/** Interrupt fires every day at noon (12:00) */
	RTC_MSPM0_INTERVAL_NOON     = 3,
};

/**
 * @brief Callback type for RTC interval timer events
 *
 * @param dev      RTC device instance
 * @param user_data User-supplied context pointer
 */
typedef void (*rtc_mspm0_interval_callback)(const struct device *dev, void *user_data);

/**
 * @brief Register a callback for the RTC interval timer (RTCTEV)
 *
 * Configures the interval at which the RTCTEV interrupt fires and registers
 * a callback to be invoked on each event. Pass @p callback as NULL to
 * disable the interval timer.
 *
 * @param dev       RTC device instance
 * @param interval  Interval selection (@ref rtc_mspm0_interval)
 * @param callback  Function to call on each interval event, or NULL to disable
 * @param user_data User context pointer passed to the callback
 *
 * @retval 0 on success
 */
int rtc_mspm0_set_interval_callback(const struct device *dev,
				    enum rtc_mspm0_interval interval,
				    rtc_mspm0_interval_callback callback,
				    void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_TI_MSPM0_H_ */
