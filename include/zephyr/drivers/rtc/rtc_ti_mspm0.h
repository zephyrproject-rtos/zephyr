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
 * @brief Prescaler timer selection
 */
enum rtc_mspm0_ps_timer {
	/** Prescaler timer 0 (RT0PS) */
	RTC_MSPM0_PS_TIMER_0 = 0,
	/** Prescaler timer 1 (RT1PS) */
	RTC_MSPM0_PS_TIMER_1 = 1,
};

/**
 * @brief RT0PS interrupt interval selection
 *
 * Valid values for the @p interval parameter of @ref rtc_mspm0_set_ps_callback
 * when @p timer is @ref RTC_MSPM0_PS_TIMER_0.
 */
enum rtc_mspm0_rt0ps_interval {
	RTC_MSPM0_RT0PS_DIV8   = 2,	/**< 244 microsecond interval  */
	RTC_MSPM0_RT0PS_DIV16  = 3,	/**< 488 microsecond interval  */
	RTC_MSPM0_RT0PS_DIV32  = 4,	/**< 976 microsecond interval  */
	RTC_MSPM0_RT0PS_DIV64  = 5,	/**< 1.95 millisecond interval */
	RTC_MSPM0_RT0PS_DIV128 = 6,	/**< 3.90 millisecond interval */
	RTC_MSPM0_RT0PS_DIV256 = 7,	/**< 7.81 millisecond interval */
};

/**
 * @brief RT1PS interrupt interval selection
 *
 * Valid values for the @p interval parameter of @ref rtc_mspm0_set_ps_callback
 * when @p timer is @ref RTC_MSPM0_PS_TIMER_1.
 */
enum rtc_mspm0_rt1ps_interval {
	RTC_MSPM0_RT1PS_DIV2   = 0,	/**< 15.6 millisecond interval */
	RTC_MSPM0_RT1PS_DIV4   = 1,	/**< 31.2 millisecond interval */
	RTC_MSPM0_RT1PS_DIV8   = 2,	/**< 62.5 millisecond interval */
	RTC_MSPM0_RT1PS_DIV16  = 3,	/**< 125 millisecond interval  */
	RTC_MSPM0_RT1PS_DIV32  = 4,	/**< 250 millisecond interval  */
	RTC_MSPM0_RT1PS_DIV64  = 5,	/**< 500 millisecond interval  */
	RTC_MSPM0_RT1PS_DIV128 = 6,	/**< 1 second interval         */
	RTC_MSPM0_RT1PS_DIV256 = 7,	/**< 2 second interval         */
};

/**
 * @brief Callback type for RTC interval timer events
 *
 * @param dev       RTC device instance
 * @param user_data User-supplied context pointer
 */
typedef void (*rtc_mspm0_interval_callback)(const struct device *dev, void *user_data);

/**
 * @brief Callback type for RTC prescaler timer events
 *
 * @param dev       RTC device instance
 * @param user_data User-supplied context pointer
 */
typedef void (*rtc_mspm0_ps_callback)(const struct device *dev, void *user_data);

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
 * @brief Register a callback for a prescaler timer (RT0PS or RT1PS)
 *
 * Configures the periodic interrupt interval for the selected prescaler timer
 * and registers a callback to be invoked on each event. Pass @p callback as
 * NULL to disable the prescaler timer interrupt.
 *
 * Use values from @ref rtc_mspm0_rt0ps_interval when @p timer is
 * @ref RTC_MSPM0_PS_TIMER_0, and values from @ref rtc_mspm0_rt1ps_interval
 * when @p timer is @ref RTC_MSPM0_PS_TIMER_1.
 *
 * @param dev       RTC device instance
 * @param timer     Prescaler timer selection (@ref rtc_mspm0_ps_timer)
 * @param interval  Interval value for the selected timer
 * @param callback  Function to call on each timer event, or NULL to disable
 * @param user_data User context pointer passed to the callback
 *
 * @retval 0 on success
 */
int rtc_mspm0_set_ps_callback(const struct device *dev,
			      enum rtc_mspm0_ps_timer timer,
			      uint8_t interval,
			      rtc_mspm0_ps_callback callback,
			      void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_TI_MSPM0_H_ */
