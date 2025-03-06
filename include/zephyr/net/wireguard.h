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

/** Wireguard VPN statistics */
struct net_stats_vpn {
	/** Number of keepalive packets received */
	uint32_t keepalive_rx;
	/** Number of keepalive packets sent */
	uint32_t keepalive_tx;
	/** Number of invalid keepalive errors */
	uint32_t invalid_keepalive;
	/** Number of handshake init packets received */
	uint32_t handshake_init_rx;
	/** Number of handshake init packets sent */
	uint32_t handshake_init_tx;
	/** Number of handshake response packets received */
	uint32_t handshake_resp_rx;
	/** Number of handshake response packets sent */
	uint32_t handshake_resp_tx;
	/** Number of invalid handshake errors */
	uint32_t invalid_handshake;
	/** Number of peer not found errors */
	uint32_t peer_not_found;
	/** Number of key expired errors */
	uint32_t key_expired;
	/** Number of invalid packets */
	uint32_t invalid_packet;
	/** Number of invalid key errors */
	uint32_t invalid_key;
	/** Number of invalid MIC errors */
	uint32_t invalid_mic;
	/** Number of invalid packet length errors */
	uint32_t invalid_packet_len;
	/** Number of invalid cookie errors */
	uint32_t invalid_cookie;
	/** Number of invalid MAC1 errors */
	uint32_t invalid_mac1;
	/** Number of invalid MAC2 errors */
	uint32_t invalid_mac2;
	/** Number of decrypt failed errors */
	uint32_t decrypt_failed;
	/** Number of dropped RX packets */
	uint32_t drop_rx;
	/** Number of dropped TX packets */
	uint32_t drop_tx;
	/** Number of allocation failures */
	uint32_t alloc_failed;
	/** Number of invalid IP version */
	uint32_t invalid_ip_version;
	/** Number of invalid IP address family */
	uint32_t invalid_ip_family;
	/** Number of denied IP address */
	uint32_t denied_ip;
	/** Number of replay errors */
	uint32_t replay_error;
	/** Number of valid packets received */
	uint32_t valid_rx;
	/** Number of valid packets sent */
	uint32_t valid_tx;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network event
 *   NET_EVENT_VPN_PEER_ADD
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 * The network event NET_EVENT_VPN_PEER_DEL will pass the peer id only
 * as a pointer in the event info.
 * Note that the pointers to the peer information are only valid during the
 * event handling and should not be used after the event is processed.
 */
struct net_event_vpn_peer {
	/** VPN peer identifier */
	uint32_t id;
	/** VPN peer public key */
	const char *public_key;
	/** VPN peer endpoint */
	struct net_sockaddr *endpoint;
	/** VPN peer allowed IP address (null terminated list) */
	struct wireguard_allowed_ip *allowed_ip[WIREGUARD_MAX_SRC_IPS + 1];
	/** VPN peer keepalive interval */
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

/**
 * @brief Get current time in seconds and nanoseconds from Unix epoch
 *
 * @details This function is used to get the current time in seconds
 *          and nanoseconds. The time is used to calculate the timestamp
 *          in the Wireguard handshake. User can override this function
 *          to provide the current time. The default implementation uses
 *          k_uptime_get() to get the current time.
 *
 * @param seconds Pointer to store the current time in seconds.
 * @param nanoseconds Pointer to store the current time in nanoseconds.
 *
 * @return 0 on success, a negative errno otherwise.
 */
int wireguard_get_current_time(uint64_t *seconds, uint32_t *nanoseconds);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_WG_H_ */
