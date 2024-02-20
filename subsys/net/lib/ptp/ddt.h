/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ddt.h
 * @brief Derived data types.
 *
 * @note Based on IEEE 1588:2019 section 5.3 - Derived data types
 */

#ifndef ZEPHYR_INCLUDE_PTP_DDT_H_
#define ZEPHYR_INCLUDE_PTP_DDT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PTP time interval in nanoseconds.
 * @note 5.3.2 - time interval expressed in nanoseconds multiplied by 2^16
 */
typedef int64_t ptp_timeinterval;

/**
 * @brief Structure for storing PTP timestamp used in PTP Protocol.
 * @note 5.3.3 - timestamp with respect to epoch
 */
struct ptp_timestamp {
	/** Seconds encoded on 48 bits - high 16 bits. */
	uint16_t seconds_high;
	/** Seconds encoded on 48 bits - low 32 bits. */
	uint32_t seconds_low;
	/** Nanoseconds. */
	uint32_t nanoseconds;
} __packed;

/**
 * @brief PTP Clock Identity.
 * @note 5.3.4 - identifies unique entities within a PTP network.
 */
typedef struct {
	/** ID bytes. */
	uint8_t id[8];
} ptp_clk_id;

/**
 * @brief PTP Port Identity.
 * @note 5.3.5 - identifies a PTP Port or a Link port.
 */
struct ptp_port_id {
	/** PTP Clock ID. */
	ptp_clk_id clk_id;
	/** PTP Port number. */
	uint16_t   port_number;
} __packed;

/**
 * @brief Structure represeniting address of a PTP Port.
 * @note 5.3.6 - represents the protocol address of a PTP port
 */
struct ptp_port_addr {
	/** PTP Port's protocol. */
	uint16_t protocol;
	/** Address length. */
	uint16_t addr_len;
	/** Address field. */
	uint8_t  address[];
} __packed;

/**
 * @brief Structure for PTP Clock quality metrics.
 * @note 5.3.7 - quality of a clock
 */
struct ptp_clk_quality {
	/** PTP Clock's class */
	uint8_t  class;
	/** Accuracy of the PTP Clock. */
	uint8_t  accuracy;
	/** Value representing stability of the Local PTP Clock. */
	uint16_t offset_scaled_log_variance;
} __packed;

/**
 * @brief
 * @note 5.3.8 - TLV (type, length, value) extension fields
 */
struct ptp_tlv {
	/** Type of the TLV value. */
	uint16_t type;
	/** Length of the TLV value field. */
	uint16_t length;
	/** TLV's data field. */
	uint8_t  value[];
} __packed;

/**
 * @brief Generic datatype for storing text in PTP messages.
 * @note 5.3.9 - holds textual content in PTP messages
 */
struct ptp_text {
	/** Length of the text field.
	 *
	 * @note Might be larger than number of symbols due to UTF-8 encoding.
	 */
	uint8_t length;
	/** Text itself.
	 *
	 * @note Encoded as UTF-8, single symbol can be 1-4 bytes long
	 */
	uint8_t text[];
} __packed;

/**
 * @brief Type holding difference between two numeric value
 * @note 5.3.11 - relative difference between two numeric values.
 * It's a dimensionless fraction and multiplied by 2^62.
 */
typedef int64_t ptp_relative_diff;

struct ptp_port;

struct ptp_clock;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_PDT_H_ */
