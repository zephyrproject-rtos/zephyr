/*
 * Copyright (c) 2018 Workaround GmbH.
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2018 Google LLC.
 * Copyright (c) 2022 Meta
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 * @brief CRC computation function
 */

#ifndef ZEPHYR_INCLUDE_CRC_NEW_CRC_NEW_H_
#define ZEPHYR_INCLUDE_CRC_NEW_CRC_NEW_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/crc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define polynomial for CRC */
#define CRC4_POLY          0x3
#define CRC4_REFLECT_POLY  0xC
#define CRC7_BE_POLY       0x09
#define CRC8_POLY          0x07
#define CRC8_REFLECT_POLY  0xE0
#define CRC16_POLY         0x8005
#define CRC16_REFLECT_POLY 0xA001
#define CRC16_CCITT_POLY   0x1021
#define CRC24_PGP_POLY     0x01864CFBU
#define CRC32_IEEE_POLY    0x04C11DB7U
#define CRC32C_POLY        0x1EDC6F41U

/**
 * @defgroup checksum Checksum
 * @ingroup os_services
 */

/**
 * @brief CRC New Function APIs
 * @defgroup crc_new_interface CRC New Function APIs
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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc16_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len, uint16_t *result);

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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc16_reflect_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len,
		      uint16_t *result);
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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc8_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result);

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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc16_ccitt_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result);

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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc16_itu_t_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result);

/**
 * @brief Compute the ANSI (or Modbus) variant of CRC-16
 *
 * The ANSI variant of CRC-16 uses 0x8005 (0xA001 reflected) as its polynomial
 * with the initial * value set to 0xffff.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
static inline int crc16_ansi_new(const uint8_t *src, size_t len, uint16_t *result)
{
	return crc16_reflect_new(0xA001, 0xffff, src, len, result);
}

/**
 * @brief Generate IEEE conform CRC32 checksum.
 *
 * @param  data         Pointer to data on which the CRC should be calculated.
 * @param  len          Data length.
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 *
 */
int crc32_ieee_new(const uint8_t *data, size_t len, uint32_t *result);

/**
 * @brief Update an IEEE conforming CRC32 checksum.
 *
 * @param crc   CRC32 checksum that needs to be updated.
 * @param data  Pointer to data on which the CRC should be calculated.
 * @param len   Data length.
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 *
 */
int crc32_ieee_update_new(uint32_t crc, const uint8_t *data, size_t len, uint32_t *result);

/**
 * @brief Calculate CRC32C (Castagnoli) checksum.
 *
 * @param crc       CRC32C checksum that needs to be updated.
 * @param data      Pointer to data on which the CRC should be calculated.
 * @param len       Data length.
 * @param first_pkt Whether this is the first packet in the stream.
 * @param last_pkt  Whether this is the last packet in the stream.
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 *
 */
int crc32_c_new(uint32_t crc, const uint8_t *data, size_t len, bool first_pkt, bool last_pkt,
		uint32_t *result);

/**
 * @brief Compute CCITT variant of CRC 8
 *
 * Normal CCITT variant of CRC 8 is using 0x07.
 *
 * @param initial_value Initial value for the CRC computation
 * @param buf Input bytes for the computation
 * @param len Length of the input in bytes
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc8_ccitt_new(uint8_t initial_value, const void *buf, size_t len, uint8_t *result);

/**
 * @brief Compute ROHC variant of CRC 8
 *
 * ROHC (Robust Header Compression) variant of CRC 8.
 * Uses 0x07 as the polynomial with reflection.
 *
 * @param initial_value Initial value for the CRC computation
 * @param buf Input bytes for the computation
 * @param len Length of the input in bytes
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc8_rohc_new(uint8_t initial_value, const void *buf, size_t len, uint8_t *result);

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
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc7_be_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result);

/**
 * @brief Compute the CRC-4 checksum of a buffer.
 *
 * Used by the TMAG5170 sensor. Uses 0x03 as the
 * polynomial with no reflection. 4 most significant
 * bits of the CRC result will be set to zero.
 *
 * @param seed Value to seed the CRC with
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc4_ti_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result);

/**
 * @brief Generic function for computing CRC 4
 *
 * Compute CRC 4 by passing in the address of the input, the input length
 * and polynomial used in addition to the initial value. The input buffer
 * must be aligned to a whole byte. It is guaranteed that 4 most significant
 * bits of the result will be set to zero.
 *
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param polynomial The polynomial to use omitting the leading x^4
 *        coefficient
 * @param initial_value Initial value for the CRC computation
 * @param reversed Should we use reflected/reversed values or not
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc4_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result);

/**
 * @brief Generate an OpenPGP CRC-24 checksum as defined in RFC 4880 section 6.1.
 *
 * @param data A pointer to the data on which the CRC will be calculated.
 * @param len Data length in bytes.
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
int crc24_pgp_new(const uint8_t *data, size_t len, uint32_t *result);

/**
 * @brief Update an OpenPGP CRC-24 checksum.
 *
 * @param crc The CRC-24 checksum that needs to be updated. The full 32-bit value of the CRC needs
 *            to be used between calls, do not mask the value to keep only the last 24 bits.
 * @param data A pointer to the data on which the CRC will be calculated.
 * @param len  Data length in bytes.
 * @param result Output parameter to hold the computed CRC result. When the last buffer of data has
 * been processed, mask the value with CRC24_FINAL_VALUE_MASK to keep only the meaningful 24 bits
 * of the CRC result.
 *
 * @return return 0 on success or error code
 */
int crc24_pgp_update_new(uint32_t crc, const uint8_t *data, size_t len, uint32_t *result);

/**
 * @brief Compute a CRC checksum, in a generic way.
 *
 * This is a dispatch function that calls the individual CRC routine
 * determined by @p type.
 *
 * For 7, 8, 16 and 24-bit CRCs, the relevant @p seed and @p poly values should
 * be passed in via the least-significant byte(s).
 *
 * Similarly, for 7, 8, 16 and 24-bit CRCs, the relevant result is stored in the
 * least-significant byte(s) of the returned value.
 *
 * @param type CRC algorithm to use.
 * @param src Input bytes for the computation
 * @param len Length of the input in bytes
 * @param seed Value to seed the CRC with
 * @param poly The polynomial to use omitting the leading coefficient
 * @param reflect Should we use reflected/reversed values or not
 * @param first Whether this is the first packet in the stream.
 * @param last Whether this is the last packet in the stream.
 * @param result Output parameter to hold the computed CRC result
 *
 * @return return 0 on success or error code
 */
static inline int crc_by_type_new(enum crc_type type, const uint8_t *src, size_t len, uint32_t seed,
				  uint32_t poly, bool reflect, bool first, bool last,
				  uint32_t *result)
{
	switch (type) {
	case CRC4:
		return crc4_new(src, len, poly, seed, reflect, (uint8_t *)result);
	case CRC4_TI:
		return crc4_ti_new(seed, src, len, (uint8_t *)result);
	case CRC7_BE:
		return crc7_be_new(seed, src, len, (uint8_t *)result);
	case CRC8:
		return crc8_new(src, len, poly, seed, reflect, (uint8_t *)result);
	case CRC8_CCITT:
		return crc8_ccitt_new(seed, src, len, (uint8_t *)result);
	case CRC8_ROHC:
		return crc8_rohc_new(seed, src, len, (uint8_t *)result);
	case CRC16:
		if (reflect) {
			return crc16_reflect_new(poly, seed, src, len, (uint16_t *)result);
		} else {
			return crc16_new(poly, seed, src, len, (uint16_t *)result);
		}
	case CRC16_ANSI:
		return crc16_ansi_new(src, len, (uint16_t *)result);
	case CRC16_CCITT:
		return crc16_ccitt_new(seed, src, len, (uint16_t *)result);
	case CRC16_ITU_T:
		return crc16_itu_t_new(seed, src, len, (uint16_t *)result);
	case CRC24_PGP: {
		uint32_t crc = crc24_pgp_update_new(seed, src, len, result);

		if (last) {
			crc &= CRC24_FINAL_VALUE_MASK;
		}
		return crc;
	}
	case CRC32_C:
		return crc32_c_new(seed, src, len, first, last, result);
	case CRC32_IEEE:
		return crc32_ieee_update_new(seed, src, len, result);
	default:
		break;
	}

	__ASSERT_NO_MSG(false);
	return -1;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
