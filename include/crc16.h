/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 * @brief CRC 16 computation function
 */

#ifndef ZEPHYR_INCLUDE_CRC16_H_
#define ZEPHYR_INCLUDE_CRC16_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup checksum Checksum
 */

/**
 * @defgroup crc16 CRC 16
 * @ingroup checksum
 * @{
 */

/**
 * @brief Generic function for computing CRC 16
 *
 * Compute CRC 16 by passing in the address of the input, the input length
 * and polynomial used in addition to the initial value.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param polynomial The polynomial to use omitting the leading x^16
 *        coefficient
 * @param initial_value Initial value for the CRC computation
 * @param pad Adds padding with zeros at the end of input bytes
 *
 * @return The computed CRC16 value
 */
u16_t crc16(const u8_t *src, size_t len, u16_t polynomial,
	    u16_t initial_value, bool pad);

/**
 * @brief Compute the CRC-16/CCITT checksum of a buffer.
 *
 * See ITU-T Recommendation V.41 (November 1988).  Uses 0x1021 as the
 * polynomial, reflects the input, and reflects the output.
 *
 * To calculate the CRC across non-contiguous blocks use the return
 * value from block N-1 as the seed for block N.
 *
 * For CRC-16/CCITT, use 0 as the initial seed.  Other checksums in
 * the same family can be calculated by changing the seed and/or
 * XORing the final value.  Examples include:
 *
 * - X-25 (used in PPP): seed=0xffff, xor=0xffff, residual=0xf0b8
 *
 * @note API changed in Zephyr 1.11.
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value
 */
u16_t crc16_ccitt(u16_t seed, const u8_t *src, size_t len);

/**
 * @brief Compute the CRC-16/XMODEM checksum of a buffer.
 *
 * The MSB first version of ITU-T Recommendation V.41 (November 1988).
 * Uses 0x1021 as the polynomial with no reflection.
 *
 * To calculate the CRC across non-contiguous blocks use the return
 * value from block N-1 as the seed for block N.
 *
 * For CRC-16/XMODEM, use 0 as the initial seed.  Other checksums in
 * the same family can be calculated by changing the seed and/or
 * XORing the final value.  Examples include:
 *
 * - CCIITT-FALSE: seed=0xffff
 * - GSM: seed=0, xorout=0xffff, residue=0x1d0f
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value
 */
u16_t crc16_itu_t(u16_t seed, const u8_t *src, size_t len);

/**
 * @brief Compute ANSI variant of CRC 16
 *
 * ANSI variant of CRC 16 is using 0x8005 as its polynomial with the initial
 * value set to 0xffff.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value
 */
static inline u16_t crc16_ansi(const u8_t *src, size_t len)
{
	return crc16(src, len, 0x8005, 0xffff, true);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
