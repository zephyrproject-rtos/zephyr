/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_ETR_H_
#define _ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_ETR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Flush data from the ETR buffer. */
void nrf_etr_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* _ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_ETR_H_ */
