/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for SWDP controller drivers
 */

#ifndef ZEPHYR_DRIVERS_DP_SWDP_COMMON_H
#define ZEPHYR_DRIVERS_DP_SWDP_COMMON_H

#include <stdint.h>
#include <zephyr/drivers/swdp.h>

/*
 * LUT to convert request value from the simplified format
 * APnDP | RnW | A[2:3]
 * to the request packet expected by the target
 * Start | APnDP | RnW | A[2:3] | Parity | Stop | Park
 */
const static uint8_t swd_request_lut[16] = {
	0x81, 0xa3, 0xa5, 0x87, 0xa9, 0x8b, 0x8d, 0xaf,
	0xb1, 0x93, 0x95, 0xb7, 0x99, 0xbb, 0xbd, 0x9f
};

/**
 * @brief Convert request from simplified format to format expected by the target
 *
 * @param[in] request value in simplified format APnDP | RnW | A[2:3]
 *
 * @return request packet expected by the target
 */
#define SWD_REQUEST_FROM_LUT(r)	(swd_request_lut[(r) & 0x0F])

/**
 * @brief Get parity of 32 bits data
 *
 * @param[in] data Transceived or received data
 *
 * @return Parity bit placed in LSB.
 */
static inline uint32_t swd_get32bit_parity(const uint32_t data)
{
	uint32_t parity = data;

	parity ^= parity >> 16;
	parity ^= parity >> 8;
	parity ^= parity >> 4;
	parity ^= parity >> 2;
	parity ^= parity >> 1;

	return parity & 1U;
}

#endif /* ZEPHYR_DRIVERS_DP_SWDP_COMMON_H */
