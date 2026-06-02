/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Datagram PLPMTUD transport-facing API
 *
 * @defgroup net_dplpmtud Datagram PLPMTUD API
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 *
 * Generic Datagram Packetization Layer Path MTU Discovery (:rfc:`8899`) for UDP
 * transports. The stack owns per-destination search state (validated PLPMTU,
 * probe size, retry counters). Each transport owns probe packet construction,
 * send with don't fragment, and confirmation through its loss-recovery path.
 *
 * Typical integration:
 *
 * 1. Call net_dplpmtud_init_path() when a connection is established.
 * 2. Use net_dplpmtud_get_path_mtu() to size outgoing datagrams.
 * 3. When net_dplpmtud_get_path_probe_size() returns a value greater than zero
 *    and net_dplpmtud_path_probe_in_flight() is false, send a probe datagram
 *    at that size with IP_DONTFRAG / IPV6_DONTFRAG enabled.
 * 4. Call net_dplpmtud_on_path_probe_sent() before sending the probe, then
 *    net_dplpmtud_on_path_probe_acked() or net_dplpmtud_on_path_probe_lost().
 *
 * ICMP Packet Too Big (PTB) updates through the PMTU destination cache are
 * applied automatically. Validated probe results are written back into that
 * cache for other stack users.
 *
 * QUIC uses this API internally; see :ref:`quic_dplpmtud`.
 */

#ifndef ZEPHYR_INCLUDE_NET_DPLPMTUD_H_
#define ZEPHYR_INCLUDE_NET_DPLPMTUD_H_

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default base PLPMTU from RFC 8899/RFC 9000 for UDP-based transports. */
#define NET_DPLPMTUD_BASE_PLPMTU 1200U

/** DPLPMTUD search state. */
enum net_dplpmtud_state {
	NET_DPLPMTUD_STATE_BASE = 0,      /**< Path is using the base PLPMTU. */
	NET_DPLPMTUD_STATE_SEARCHING,     /**< Path is actively probing for a larger PLPMTU. */
	NET_DPLPMTUD_STATE_SEARCH_COMPLETE, /**< Path has no larger probe candidate. */
	NET_DPLPMTUD_STATE_ERROR,         /**< Path fell back after black-hole detection. */
};

/** Transport-owned DPLPMTUD path handle. */
struct net_dplpmtud_path {
	/** Remote destination for this path. */
	struct net_sockaddr_storage dst;

	/** Transport-local maximum PLPMTU, or UINT16_MAX if uncapped. */
	uint16_t max_plpmtu;

	/** Path handle is initialized. */
	bool in_use;
};

/**
 * @brief Initialize a DPLPMTUD path handle for a remote destination.
 *
 * @param path Path handle to initialize
 * @param dst Remote destination address
 * @param max_plpmtu Transport-local maximum PLPMTU, or 0 if uncapped
 *
 * @retval 0 on success
 * @retval -EINVAL if an argument is invalid
 * @retval -ENOMEM if no DPLPMTUD state entry can be created
 */
int net_dplpmtud_init_path(struct net_dplpmtud_path *path,
			   const struct net_sockaddr *dst,
			   uint16_t max_plpmtu);

/**
 * @brief Update the transport-local maximum PLPMTU for a path.
 *
 * @param path Initialized path handle
 * @param max_plpmtu Transport-local maximum PLPMTU, or 0 if uncapped
 */
void net_dplpmtud_set_path_max_plpmtu(struct net_dplpmtud_path *path,
				      uint16_t max_plpmtu);

/**
 * @brief Get the currently validated PLPMTU for a path.
 *
 * The path must have been initialized with net_dplpmtud_init_path(). This
 * call does not create per-destination DPLPMTUD state.
 *
 * @param path Initialized path handle
 *
 * @return Validated PLPMTU on success, negative errno otherwise
 */
int net_dplpmtud_get_path_mtu(struct net_dplpmtud_path *path);

/**
 * @brief Get the next probe size a transport may transmit for a path.
 *
 * The path must have been initialized with net_dplpmtud_init_path(). This
 * call does not create per-destination DPLPMTUD state.
 *
 * @param path Initialized path handle
 *
 * @return Next probe size, 0 if no probe is currently needed, negative errno on error
 */
int net_dplpmtud_get_path_probe_size(struct net_dplpmtud_path *path);

/**
 * @brief Check whether a probe is currently in flight for a path.
 *
 * The path must have been initialized with net_dplpmtud_init_path(). This
 * call does not create per-destination DPLPMTUD state.
 *
 * @param path Initialized path handle
 *
 * @return true if a probe is in flight, false otherwise
 */
bool net_dplpmtud_path_probe_in_flight(struct net_dplpmtud_path *path);

/**
 * @brief Report that a probe has been sent for a path.
 *
 * @param path Initialized path handle
 * @param probe_size Probe size sent by the transport
 *
 * @retval 0 on success
 * @retval -EINVAL if an argument is invalid
 * @retval -EMSGSIZE if the probe exceeds the transport-local path limit
 * @retval negative errno from the DPLPMTUD core otherwise
 */
int net_dplpmtud_on_path_probe_sent(struct net_dplpmtud_path *path,
				    uint16_t probe_size);

/**
 * @brief Report that a probe was acknowledged for a path.
 *
 * @param path Initialized path handle
 * @param probe_size Probe size acknowledged by the transport
 *
 * @retval 0 on success
 * @retval negative errno otherwise
 */
int net_dplpmtud_on_path_probe_acked(struct net_dplpmtud_path *path,
				     uint16_t probe_size);

/**
 * @brief Report that a probe was lost for a path.
 *
 * @param path Initialized path handle
 * @param probe_size Probe size declared lost by the transport
 *
 * @retval 0 on success
 * @retval negative errno otherwise
 */
int net_dplpmtud_on_path_probe_lost(struct net_dplpmtud_path *path,
				    uint16_t probe_size);

/**
 * @brief Report black-hole fallback for a path.
 *
 * Resets the validated PLPMTU toward the base size, stops probing, and
 * records the path in the error state. Transports may also call this when
 * they detect a black hole without an ICMP PTB.
 *
 * @param path Initialized path handle
 */
void net_dplpmtud_note_path_blackhole(struct net_dplpmtud_path *path);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_DPLPMTUD_H_ */
