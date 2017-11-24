/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 * @brief CRC 16 computation function
 */

#ifndef __CRC16_H
#define __CRC16_H

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

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
 * @brief Compute CCITT variant of CRC 16
 *
 * CCITT variant of CRC 16 is using 0x1021 as its polynomial with the initial
 * value set to 0xffff.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value
 */
static inline u16_t crc16_ccitt(const u8_t *src, size_t len)
{
	return crc16(src, len, 0x1021, 0xffff, true);
}

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
#endif
