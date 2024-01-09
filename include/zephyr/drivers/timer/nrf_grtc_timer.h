/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_GRTC_TIMER_H
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_GRTC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys_clock.h>

/** @brief GRTC timer compare event handler.
 *
 * Called from GRTC ISR context when processing a compare event.
 *
 * @param id Compare channel ID.
 *
 * @param expire_time An actual absolute expiration time set for a compare
 *		      channel. It can differ from the requested target time
 *		      and the difference can be used to determine whether the
 *		      time set was delayed.
 *
 * @param user_data Pointer to a user context data.
 */
typedef void (*z_nrf_grtc_timer_compare_handler_t)(int32_t id, uint64_t expire_time,
						   void *user_data);

/** @brief Allocate GRTC capture/compare channel.
 *
 * @retval >=0 Non-negative indicates allocated channel ID.
 * @retval -ENOMEM if channel cannot be allocated.
 */
int32_t z_nrf_grtc_timer_chan_alloc(void);

/** @brief Free GRTC capture/compare channel.
 *
 * @param chan Previously allocated channel ID.
 */
void z_nrf_grtc_timer_chan_free(int32_t chan);

/** @brief Read current absolute time.
 *
 * @return Current absolute time.
 */
uint64_t z_nrf_grtc_timer_read(void);

/** @brief Check COMPARE event state.
 *
 * @param chan Channel ID.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
bool z_nrf_grtc_timer_compare_evt_check(int32_t chan);

/** @brief Get COMPARE event register address.
 *
 * Address can be used for DPPIC.
 *
 * @param chan Channel ID.
 *
 * @return Register address.
 */
uint32_t z_nrf_grtc_timer_compare_evt_address_get(int32_t chan);

/** @brief Get CAPTURE task register address.
 *
 * Address can be used for DPPIC.
 *
 * @param chan Channel ID.
 *
 * @return Register address.
 */
uint32_t z_nrf_grtc_timer_capture_task_address_get(int32_t chan);

/** @brief Safely disable compare event interrupt.
 *
 * Function returns key indicating whether interrupt was already disabled.
 *
 * @param chan Channel ID.
 *
 * @return key passed to z_nrf_grtc_timer_compare_int_unlock().
 */
bool z_nrf_grtc_timer_compare_int_lock(int32_t chan);

/** @brief Safely enable compare event interrupt.
 *
 * Event interrupt is conditionally enabled based on @p key.
 *
 * @param chan Channel ID.
 *
 * @param key Key returned by z_nrf_grtc_timer_compare_int_lock().
 */
void z_nrf_grtc_timer_compare_int_unlock(int32_t chan, bool key);

/** @brief Read compare register value.
 *
 * @param chan Channel ID.
 *
 * @retval >=0 Positive is a Value set in the compare register
 * @retval -EAGAIN if compare for given channel is not set.
 * @retval -EPERM if either channel is unavailable or SYSCOUNTER is not running.
 */
uint64_t z_nrf_grtc_timer_compare_read(int32_t chan);

/** @brief  Set compare channel to given value.
 *
 * @param chan Channel ID.
 *
 * @param target_time Absolute target time in ticks.
 *
 * @param handler User function called in the context of the GRTC interrupt.
 *
 * @param user_data Data passed to the handler.
 *
 * @retval 0 if the compare channel was set successfully.
 * @retval -EPERM if either channel is unavailable or SYSCOUNTER is not running.
 */
int z_nrf_grtc_timer_set(int32_t chan, uint64_t target_time,
			 z_nrf_grtc_timer_compare_handler_t handler, void *user_data);

/** @brief Abort a timer requested with z_nrf_grtc_timer_set().
 *
 * If an abort operation is performed too late it is still possible for an event
 * to fire. The user can detect a spurious event by comparing absolute time
 * provided in callback and a result of z_nrf_grtc_timer_read(). During this
 * operation interrupt from that compare channel is disabled. Other interrupts
 * are not locked during this operation.
 *
 * @param chan Channel ID between 1 and CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT.
 */
void z_nrf_grtc_timer_abort(int32_t chan);

/** @brief Convert system clock time to GRTC ticks.
 *
 * @p t can be absolute or relative.
 *
 * @retval >=0 Positive value represents @p t in GRTC tick value.
 * @retval -EINVAL if @p t is out of range.
 */
uint64_t z_nrf_grtc_timer_get_ticks(k_timeout_t t);

/** @brief Prepare channel for timestamp capture.
 *
 * Use z_nrf_grtc_timer_capture_task_address_get() to determine the register
 * address that is used to trigger capture.
 *
 * @note Capture and compare are mutually exclusive features - they cannot be
 *       used simultaneously on the same GRTC channel.
 *
 * @param chan Channel ID.
 *
 * @retval 0 if the channel was successfully prepared.
 * @retval -EPERM if either channel is unavailable or SYSCOUNTER is not running.
 */
int z_nrf_grtc_timer_capture_prepare(int32_t chan);

/** @brief Read timestamp value captured on the channel.
 *
 * The @p chan must be prepared using z_nrf_grtc_timer_capture_prepare().
 *
 * @param chan Channel ID.
 *
 * @param captured_time Pointer to store the value.
 *
 * @retval 0 if the timestamp was successfully caught and read.
 * @retval -EBUSY if capturing has not been triggered.
 */
int z_nrf_grtc_timer_capture_read(int32_t chan, uint64_t *captured_time);

/** @brief Prepare GRTC as a source of wake up event and set the wake up time.
 *
 * @note Calling this function should be immediately followed by low-power mode enter
 *		(if it executed successfully).
 *
 * @param wake_time_us Relative wake up time in microseconds.
 *
 * @retval 0 if wake up time was successfully set.
 * @retval -EPERM if the SYSCOUNTER is not running.
 * @retval -ENOMEM if no available GRTC channels were found.
 * @retval -EINVAL if @p wake_time_us is too low.
 */
int z_nrf_grtc_wakeup_prepare(uint64_t wake_time_us);

/**
 * @brief       Initialize the GRTC clock timer driver from an application-
 *              defined function.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on failure.
 */
int nrf_grtc_timer_clock_driver_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_NRF_GRTC_TIMER_H */
