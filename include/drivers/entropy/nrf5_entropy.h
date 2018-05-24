/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NRF5_ENTROPY_H_
#define _NRF5_ENTROPY_H_

/**
 * @brief Fills a buffer with entropy in a non-blocking manner.
 * 	  Callable from ISRs.
 *
 * @param dev Pointer to the device structure.
 * @param buf Buffer to fill with entropy.
 * @param len Buffer length.
 * @retval number of bytes filled with entropy.
 */
u8_t entropy_nrf_get_entropy_isr(struct device *dev, u8_t *buf, u8_t len);

#endif /* _NRF5_ENTROPY_H_ */
