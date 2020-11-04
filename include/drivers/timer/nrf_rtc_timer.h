/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_RTC_TIMER_H
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_RTC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*z_nrf_rtc_timer_compare_handler_t)(uint32_t id,
						uint32_t cc_value,
						void *user_data);

/** @brief Allocate RTC compare channel.
 *
 * Channel 0 is used for the system clock.
 *
 * @retval Non-negative indicates allocated channel ID.
 * @retval -ENOMEM if channel cannot be allocated.
 */
int z_nrf_rtc_timer_chan_alloc(void);

/** @brief Free RTC compare channel.
 *
 * @param chan Previously allocated channel ID.
 */
void z_nrf_rtc_timer_chan_free(uint32_t chan);

/** @brief Read current RTC counter value.
 *
 * @return Current RTC counter value.
 */
uint32_t z_nrf_rtc_timer_read(void);

/** @brief Get COMPARE event register address.
 *
 * Address can be used for (D)PPI.
 *
 * @param chan Channel ID between 0 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 *
 * @return Register address.
 */
uint32_t z_nrf_rtc_timer_compare_evt_address_get(uint32_t chan);

/** @brief Safely disable compare event interrupt.
 *
 * Function returns key indicating whether interrupt was already disabled.
 *
 * @param chan Channel ID between 1 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 *
 * @return key passed to @ref z_nrf_rtc_timer_compare_int_unlock.
 */
bool z_nrf_rtc_timer_compare_int_lock(uint32_t chan);

/** @brief Safely enable compare event interrupt.
 *
 * Event interrupt is conditionally enabled based on @p key.
 *
 * @param chan Channel ID between 1 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 *
 * @param key Key returned by @ref z_nrf_rtc_timer_compare_int_lock.
 */
void z_nrf_rtc_timer_compare_int_unlock(uint32_t chan, bool key);

/** @brief Read compare register value.
 *
 * @param chan Channel ID between 0 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 *
 * @return Value set in the compare register.
 */
uint32_t z_nrf_rtc_timer_compare_read(uint32_t chan);

/** @brief  Try to set compare channel to given value.
 *
 * Provided value is absolute and cannot be further in future than half span of
 * the RTC counter. Function continouosly retries to set compare register until
 * value that is written is far enough in the future and will generate an event.
 * Because of that, compare register value may be different than the one
 * requested. During this operation interrupt from that compare channel is
 * disabled. Other interrupts are not locked during this operation.
 *
 * There is no option to abort the request once it is set. However, it can be
 * overwritten.
 *
 * @param chan Channel ID between 1 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 *
 * @param cc_value Absolute value. Values which are further distanced from
 * current counter value than half RTC span are considered in the past.
 *
 * @param handler User function called in the context of the RTC interrupt.
 *
 * @param user_data Data passed to the handler.
 */
void z_nrf_rtc_timer_compare_set(uint32_t chan, uint32_t cc_value,
			       z_nrf_rtc_timer_compare_handler_t handler,
			       void *user_data);

/** @brief Convert system clock time to RTC ticks.
 *
 * @p t can be absolute or relative. @p t cannot be further from now than half
 * of the RTC range (e.g. 256 seconds if RTC is running at 32768 Hz).
 *
 * @retval Positive value represents @p t in RTC tick value.
 * @retval -EINVAL if @p t is out of range.
 */
int z_nrf_rtc_timer_get_ticks(k_timeout_t t);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_RTC_TIMER_H */
