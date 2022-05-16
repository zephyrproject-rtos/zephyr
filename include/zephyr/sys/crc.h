/*
 * Copyright (c) 2018 Workaround GmbH.
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 * @brief CRC computation function
 */

#ifndef ZEPHYR_INCLUDE_SYS_CRC_H_
#define ZEPHYR_INCLUDE_SYS_CRC_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initial value expected to be used at the beginning of the crc8_ccitt
 * computation.
 */
#define CRC8_CCITT_INITIAL_VALUE 0xFF

/**
 * @defgroup checksum Checksum
 */

/**
 * @defgroup crc CRC
 * @ingroup checksum
 * @{
 */

/**
 * @brief Generic function for computing a CRC-16 without input or output
 *        reflection.
 *
 * Compute CRC-16 by passing in the address of the input, the input length
 * and polynomial used in addition to the initial value. This is O(n*8) where n
 * is the length of the buffer provided. No reflection is performed.
 *
 * @note If you are planning to use a CRC based on poly 0x1012 the functions
 * crc16_itu_t() is faster and thus recommended over this one.
 *
 * @param poly The polynomial to use omitting the leading x^16
 *             coefficient
 * @param seed Initial value for the CRC computation
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value (without any XOR applied to it)
 */
uint16_t crc16(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len);

/**
 * @brief Generic function for computing a CRC-16 with input and output
 *        reflection.
 *
 * Compute CRC-16 by passing in the address of the input, the input length
 * and polynomial used in addition to the initial value. This is O(n*8) where n
 * is the length of the buffer provided. Both input and output are reflected.
 *
 * @note If you are planning to use a CRC based on poly 0x1012 the function
 * crc16_ccitt() is faster and thus recommended over this one.
 *
 * The following checksums can, among others, be calculated by this function,
 * depending on the value provided for the initial seed and the value the final
 * calculated CRC is XORed with:
 *
 * - CRC-16/ANSI, CRC-16/MODBUS, CRC-16/USB, CRC-16/IBM
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-modbus
 *   poly: 0x8005 (0xA001) initial seed: 0xffff, xor output: 0x0000
 *
 * @param poly The polynomial to use omitting the leading x^16
 *             coefficient. Important: please reflect the poly. For example,
 *             use 0xA001 instead of 0x8005 for CRC-16-MODBUS.
 * @param seed Initial value for the CRC computation
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value (without any XOR applied to it)
 */
uint16_t crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len);
/**
 * @brief Generic function for computing CRC 8
 *
 * Compute CRC 8 by passing in the address of the input, the input length
 * and polynomial used in addition to the initial value.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param polynomial The polynomial to use omitting the leading x^8
 *        coefficient
 * @param initial_value Initial value for the CRC computation
 * @param reversed Should we use reflected/reversed values or not
 *
 * @return The computed CRC8 value
 */
uint8_t crc8(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	  bool reversed);

/**
 * @brief Compute the checksum of a buffer with polynomial 0x1021, reflecting
 *        input and output.
 *
 * This function is able to calculate any CRC that uses 0x1021 as it polynomial
 * and requires reflecting both the input and the output. It is a fast variant
 * that runs in O(n) time, where n is the length of the input buffer.
 *
 * The following checksums can, among others, be calculated by this function,
 * depending on the value provided for the initial seed and the value the final
 * calculated CRC is XORed with:
 *
 * - CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/KERMIT
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-kermit
 *   initial seed: 0x0000, xor output: 0x0000
 *
 * - CRC-16/X-25, CRC-16/IBM-SDLC, CRC-16/ISO-HDLC
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-sdlc
 *   initial seed: 0xffff, xor output: 0xffff
 *
 * @note To calculate the CRC across non-contiguous blocks use the return
 *       value from block N-1 as the seed for block N.
 *
 * See ITU-T Recommendation V.41 (November 1988).
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value (without any XOR applied to it)
 */
uint16_t crc16_ccitt(uint16_t seed, const uint8_t *src, size_t len);

/**
 * @brief Compute the checksum of a buffer with polynomial 0x1021, no
 *        reflection of input or output.
 *
 * This function is able to calculate any CRC that uses 0x1021 as it polynomial
 * and requires no reflection on  both the input and the output. It is a fast
 * variant that runs in O(n) time, where n is the length of the input buffer.
 *
 * The following checksums can, among others, be calculated by this function,
 * depending on the value provided for the initial seed and the value the final
 * calculated CRC is XORed with:
 *
 * - CRC-16/XMODEM, CRC-16/ACORN, CRC-16/LTE
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-xmodem
 *   initial seed: 0x0000, xor output: 0x0000
 *
 * - CRC16/CCITT-FALSE, CRC-16/IBM-3740, CRC-16/AUTOSAR
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-3740
 *   initial seed: 0xffff, xor output: 0x0000
 *
 * - CRC-16/GSM
 *   https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-gsm
 *   initial seed: 0x0000, xor output: 0xffff
 *
 * @note To calculate the CRC across non-contiguous blocks use the return
 *       value from block N-1 as the seed for block N.
 *
 * See ITU-T Recommendation V.41 (November 1988) (MSB first).
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value (without any XOR applied to it)
 */
uint16_t crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len);

/**
 * @brief Compute the ANSI (or Modbus) variant of CRC-16
 *
 * The ANSI variant of CRC-16 uses 0x8005 (0xA001 reflected) as its polynomial
 * with the initial * value set to 0xffff.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC16 value
 */
static inline uint16_t crc16_ansi(const uint8_t *src, size_t len)
{
	return crc16_reflect(0xA001, 0xffff, src, len);
}

/**
 * @brief Generate IEEE conform CRC32 checksum.
 *
 * @param  *data        Pointer to data on which the CRC should be calculated.
 * @param  len          Data length.
 *
 * @return CRC32 value.
 *
 */
uint32_t crc32_ieee(const uint8_t *data, size_t len);

/**
 * @brief Update an IEEE conforming CRC32 checksum.
 *
 * @param crc   CRC32 checksum that needs to be updated.
 * @param *data Pointer to data on which the CRC should be calculated.
 * @param len   Data length.
 *
 * @return CRC32 value.
 *
 */
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * @brief Calculate CRC32C (Castagnoli) checksum.
 *
 * @param crc       CRC32C checksum that needs to be updated.
 * @param *data     Pointer to data on which the CRC should be calculated.
 * @param len       Data length.
 * @param first_pkt Whether this is the first packet in the stream.
 * @param last_pkt  Whether this is the last packet in the stream.
 *
 * @return CRC32 value.
 *
 */
uint32_t crc32_c(uint32_t crc, const uint8_t *data,
		 size_t len, bool first_pkt, bool last_pkt);

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
uint8_t crc8_ccitt(uint8_t initial_value, const void *buf, size_t len);

/**
 * @brief Compute the CRC-7 checksum of a buffer.
 *
 * See JESD84-A441.  Used by the MMC protocol.  Uses 0x09 as the
 * polynomial with no reflection.  The CRC is left
 * justified, so bit 7 of the result is bit 6 of the CRC.
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 *
 * @return The computed CRC7 value
 */
uint8_t crc7_be(uint8_t seed, const uint8_t *src, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
