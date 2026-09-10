/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/**
 * @brief Request lowest latency for system events
 *
 * @details System will be configured for lowest latency after first
 * call to nrf_sys_event_request_global_constlat() and will remain
 * configured for lowest latency until matching number of calls to
 * nrf_sys_event_release_global_constlat() occur.
 *
 * @retval 0 if successful
 * @retval -errno code otherwise
 */
int nrf_sys_event_request_global_constlat(void);

/**
 * @brief Release low latency request
 *
 * @see nrf_sys_event_request_global_constlat()
 *
 * @retval 0 if successful
 * @retval -errno code otherwise
 */
int nrf_sys_event_release_global_constlat(void);

/**
 * @brief Register an event (interrupt) using relative time.
 *
 * Registering an event allows system to prepare for wake up minimizing power consumption
 * and latency. There are 2 ways of minimizing interrupt latency:
 * - Setting low latency power mode for NVM memory. This option is not available in non-secure
 *   build.
 * - Using DPPI to wake up memory before the expected interrupt (if enabled). This option requires
 *   additional resource (DPPI and GRTC) which are allocated during registration and freed in
 *   nrf_sys_event_unregister. If resources are not available then algorithm falls back
 *   to the option 1. Timing should be precise as if memory is woken up it will go back to
 *   sleep if it is not used for certain amount of time (in case of RRAMC 4 us by default).
 *   CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT configures target amount of GRTC channels. During
 *   the initialization those channels are dynamically allocated so it is possible that less
 *   are available.
 *
 * @param us Time (in microseconds) from now when interrupt is expected to be triggered.
 *
 * @param force If true then low latency mode if forced when there is no resources available
 * for NVM memory wake up.
 *
 * @retval non-negative Handle which shall be used to unregister the event.
 * @retval -ENOTSUP Feature is not supported.
 */
int nrf_sys_event_register(uint32_t us, bool force);

/**
 * @brief Register an event (interrupt) using absolute time.
 *
 * See nrf_sys_event_register for more details.
 *
 * @param us Absolute time (in microseconds) when interrupt is expected to be triggered.
 *
 * @param force If true then low latency mode if forced when there is no resources available
 * for NVM memory wake up.
 *
 * @retval non-negative Handle which shall be used to unregister the event.
 * @retval -ENOTSUP Feature is not supported.
 */
int nrf_sys_event_abs_register(uint64_t us, bool force);

/** @brief Unregister an event.
 *
 * It must be called after the registered event occurred or if it was canceled.
 *
 * @param handle Handle returned by @ref nrf_sys_event_register.
 * @param cancel True to indicate that event is unregistered due to cancellation. Setting to true
 * when event executed results only in slight longer duration of the operation.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Invalid handle.
 * @retval -ENOTSUP Not supported.
 */
int nrf_sys_event_unregister(int handle, bool cancel);
