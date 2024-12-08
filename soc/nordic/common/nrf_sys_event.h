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
