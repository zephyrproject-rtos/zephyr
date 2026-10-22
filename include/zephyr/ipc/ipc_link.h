/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IPC link backend: the transport interface
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_LINK_H_
#define ZEPHYR_INCLUDE_IPC_IPC_LINK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC link backend transport interface
 * @defgroup ipc_link_api IPC link transport interface
 * @ingroup ipc
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Counters of a link
 *
 * Kept when CONFIG_IPC_SERVICE_LINK_STATS is enabled, read with ipc_link_stats_get() and
 * cleared with ipc_link_stats_reset().
 */
struct ipc_link_stats {
	/** Frames received */
	uint32_t rx_frames;
	/** Payloads sent for the first time */
	uint32_t tx_payloads;
	/** Payloads sent again */
	uint32_t retransmits;
	/** Payloads received in order and delivered */
	uint32_t rx_payloads;
	/** Failed transfers and headers with a bad CRC */
	uint32_t header_errors;
	/** Headers with another protocol version or frame size */
	uint32_t peer_mismatches;
	/** Payloads with a bad CRC or layout */
	uint32_t payload_errors;
	/** Valid payloads dropped, out of order or without a free receive buffer */
	uint32_t rx_dropped;
	/** Peer reboots */
	uint32_t peer_resets;
	/** Sessions ended after too many timeouts, or by an incompatible peer */
	uint32_t disconnects;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_LINK_H_ */
