/**
 * @file
 * @brief Wireguard VPN
 *
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WG_H_
#define ZEPHYR_INCLUDE_NET_WG_H_

/**
 * @brief Wireguard VPN service
 * @defgroup wg_vpn_service Wireguard VPN service
 * @since 4.4
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_WIREGUARD)
#define WIREGUARD_INTERFACE CONFIG_WIREGUARD_INTERFACE
#else
#define WIREGUARD_INTERFACE ""
#endif

#if defined(CONFIG_WIREGUARD_MAX_SRC_IPS)
#define WIREGUARD_MAX_SRC_IPS CONFIG_WIREGUARD_MAX_SRC_IPS
#else
#define WIREGUARD_MAX_SRC_IPS 1
#endif

/** @endcond */

/** Timestamp length (64-bit seconds and 32-bit nanoseconds) */
#define WIREGUARD_TIMESTAMP_LEN (sizeof(uint64_t) + sizeof(uint32_t))

/** @brief Wireguard allowed IP address struct */
struct wireguard_allowed_ip {
	/** Allowed IPv4 or IPv6 address */
	struct net_addr addr;
	/** Netmask (for IPv4) or Prefix (for IPv6) length */
	uint8_t mask_len;
	/** Is the allowed IP address valid */
	bool is_valid : 1;
};

/**
 * @brief Wireguard peer configuration information.
 *
 * Stores the Wireguard VPN peer connection information.
 */
struct wireguard_peer_config {
	/** Public key in base64 format */
	const char *public_key;

	/** Optional pre-shared key (32 bytes), set to NULL if not to be used */
	const uint8_t *preshared_key;

	/** What is the largest timestamp we have seen during handshake in order
	 * to avoid replays.
	 */
	uint8_t timestamp[WIREGUARD_TIMESTAMP_LEN];

	/** End-point address (when connecting) */
	struct net_sockaddr_storage endpoint_ip;

	/** Allowed IP address */
	struct wireguard_allowed_ip allowed_ip[WIREGUARD_MAX_SRC_IPS];

	/** Default keep alive time for this peer in seconds.
	 * Set to 0 to use the default value.
	 */
	int keepalive_interval;
};

/**
 * @brief Add a Wireguard peer to the system.
 *
 * @details If successful, a virtual network interface is
 *          returned which can be used to communicate with the peer.
 *
 * @param peer_config Peer configuration data.
 * @param peer_iface A pointer to network interface is returned to the
 *        caller if adding the peer was successful.
 *
 * @return >0 peer id on success, a negative errno otherwise.
 */
int wireguard_peer_add(struct wireguard_peer_config *peer_config,
		       struct net_if **peer_iface);

/**
 * @brief Remove a Wireguard peer from the system.
 *
 * @details If successful, the virtual network interface is
 *          also removed and user is no longer be able to communicate
 *          with the peer.
 *
 * @param peer_id Peer id returned by wireguard_peer_add()
 *
 * @return 0 on success, a negative errno otherwise.
 */
int wireguard_peer_remove(int peer_id);

/**
 * @brief Send a Wireguard keepalive message to peer.
 *
 * @param peer_id Peer id returned by wireguard_peer_add()
 *
 * @return 0 on success, a negative errno otherwise.
 */
int wireguard_peer_keepalive(int peer_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_WG_H_ */
