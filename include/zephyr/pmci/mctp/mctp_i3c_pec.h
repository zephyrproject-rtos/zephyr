/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I3C_PEC_H_
#define ZEPHYR_MCTP_I3C_PEC_H_

/**
 * @file
 * @brief MCTP-over-I3C Packet Error Code (PEC) helpers.
 *
 * Internal helpers for computing and verifying the CRC-8 based Packet Error
 * Code appended to MCTP-over-I3C packets as defined by DSP0233.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Size of the trailing Packet Error Code byte per DSP0233.
 */
#define I3C_PROTOCOL_PEC_SZ	1U

/**
 * @brief Calculate the MCTP-over-I3C Packet Error Code (PEC).
 *
 * Computes the PEC as defined by DSP0233 a CRC-8 (CCITT, poly 0x07)
 * seeded with the I3C address byte (dynamic_addr << 1 | R/W) and computed
 * over the payload bytes.
 *
 * @param buf       Payload bytes the PEC is computed over (PEC byte excluded).
 * @param len       Number of payload bytes in @p buf.
 * @param addr_byte I3C address byte used to seed the CRC.
 *
 * @return The calculated PEC byte.
 */
uint8_t mctp_i3c_calculate_pec(const uint8_t *buf, size_t len, uint8_t addr_byte);

/**
 * @brief Verify the MCTP-over-I3C Packet Error Code (PEC).
 *
 * Recomputes the PEC over buffer (excluding its trailing PEC byte) and compares
 * it against the received PEC byte at buf[len - I3C_PROTOCOL_PEC_SZ].
 *
 * @param buf       Received bytes, including the trailing PEC byte.
 * @param len       Total number of bytes in @p buf, including the PEC byte.
 * @param addr_byte I3C address byte used to seed the CRC.
 *
 * @retval 0 PEC matches.
 * @retval -EBADMSG Buffer too short to contain a PEC, or PEC mismatch.
 */
int mctp_i3c_verify_pec(const uint8_t *buf, size_t len, uint8_t addr_byte);

/** @endcond INTERNAL_HIDDEN */

#endif /* ZEPHYR_MCTP_I3C_PEC_H_ */
