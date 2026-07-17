/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DEBUG_DEBUG_NRF_ETR_H_
#define ZEPHYR_INCLUDE_DRIVERS_DEBUG_DEBUG_NRF_ETR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Flush data from the ETR buffer. */
void debug_nrf_etr_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DEBUG_DEBUG_NRF_ETR_H_ */
