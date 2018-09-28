/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CRC8_H_
#define ZEPHYR_INCLUDE_CRC8_H_

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initial value expected to be used at the beginning of the crc8_ccitt
 * computation.
 */
#define CRC8_CCITT_INITIAL_VALUE 0xFF

/**
 * @brief Compute CCITT variant of CRC 8
 *
 * Normal CCITT variant of CRC 8 is using 0x07.
 *
 * @param initial_value Initial value for the CRC computation
 * @param buf Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC8 value
 */
u8_t crc8_ccitt(u8_t initial_value, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
