/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Rapid Spanning Tree Protocol internals
 *
 * Shared between the RSTP sources and the tests. Not part of the public API.
 * Clause and figure numbers refer to IEEE Std 802.1Q-2014, which carries the
 * RSTP of IEEE Std 802.1D-2004 in Clause 13 and the BPDU formats in Clause 14.
 */

#ifndef ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Length of an STP TCN BPDU (Figure 14-2) */
#define ETH_RSTP_BPDU_TCN_LEN    4U
/** Length of an STP Configuration BPDU (14.4) */
#define ETH_RSTP_BPDU_CONFIG_LEN 35U
/** Length of an RST BPDU (14.4) */
#define ETH_RSTP_BPDU_RST_LEN    36U
/** Longest BPDU that is encoded */
#define ETH_RSTP_BPDU_MAX_LEN    ETH_RSTP_BPDU_RST_LEN

/** BPDU flags, octet 5 (14.4) */
#define ETH_RSTP_FLAG_TC         BIT(0)
#define ETH_RSTP_FLAG_PROPOSAL   BIT(1)
#define ETH_RSTP_FLAG_ROLE_SHIFT 2U
#define ETH_RSTP_FLAG_ROLE_MASK  (BIT(2) | BIT(3))
#define ETH_RSTP_FLAG_LEARNING   BIT(4)
#define ETH_RSTP_FLAG_FORWARDING BIT(5)
#define ETH_RSTP_FLAG_AGREEMENT  BIT(6)
#define ETH_RSTP_FLAG_TC_ACK     BIT(7)

/** Port Role values in the flags (14.2.9) */
#define ETH_RSTP_BPDU_ROLE_UNKNOWN    0U
#define ETH_RSTP_BPDU_ROLE_ALT_BACKUP 1U
#define ETH_RSTP_BPDU_ROLE_ROOT       2U
#define ETH_RSTP_BPDU_ROLE_DESIGNATED 3U

/** BPDU types after validation (14.5) */
enum eth_rstp_bpdu_type {
	ETH_RSTP_BPDU_CONFIG,
	ETH_RSTP_BPDU_TCN,
	ETH_RSTP_BPDU_RST,
};

/** Timer values of a Configuration Message, in seconds */
struct eth_rstp_times {
	uint16_t message_age;
	uint16_t max_age;
	uint16_t hello_time;
	uint16_t forward_delay;
};

/** A decoded BPDU. A TCN BPDU uses only @p type. */
struct eth_rstp_bpdu {
	enum eth_rstp_bpdu_type type;
	/** ETH_RSTP_FLAG_* bits that are defined for @p type */
	uint8_t flags;
	uint64_t root_id;
	uint32_t root_path_cost;
	uint64_t bridge_id;
	uint16_t port_id;
	struct eth_rstp_times times;
};

/**
 * @brief Validate and decode a received BPDU
 *
 * @param buf BPDU, starting at the Protocol Identifier
 * @param len Length of @p buf
 * @param bpdu Decoded BPDU
 *
 * @retval 0 The BPDU is valid and was decoded.
 * @retval -EINVAL The BPDU is not a Configuration, TCN or RST BPDU (14.5 h).
 */
int eth_rstp_bpdu_decode(const uint8_t *buf, size_t len, struct eth_rstp_bpdu *bpdu);

/**
 * @brief Encode a BPDU for transmission
 *
 * @param bpdu BPDU to encode
 * @param buf Buffer for the BPDU, starting at the Protocol Identifier
 * @param len Size of @p buf
 *
 * @return Length of the encoded BPDU, or 0 if @p buf is too short.
 */
size_t eth_rstp_bpdu_encode(const struct eth_rstp_bpdu *bpdu, uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_ */
