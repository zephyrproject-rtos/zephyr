/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_WIREGUARD_WG_PRIVATE_H_
#define ZEPHYR_SUBSYS_NET_LIB_WIREGUARD_WG_PRIVATE_H_

#include <zephyr/net/wireguard.h>

struct net_pkt;

/** @brief Public configuration snapshot of a WireGuard peer. */
struct wireguard_peer_info {
	/** Raw 32-byte WireGuard public key. */
	uint8_t public_key[32];
	/** Configured remote UDP endpoint. */
	struct net_sockaddr_storage endpoint;
	/** Configured allowed prefixes. */
	struct wireguard_allowed_ip allowed_ip[WIREGUARD_MAX_SRC_IPS];
	/** Configured keepalive interval in seconds. */
	uint16_t keepalive_interval;
};

/** @brief Copy a peer's public configuration into caller-owned storage.
 * @param peer_id Identifier returned when the peer was added.
 * @param info Destination for the configuration snapshot.
 * @retval 0 Snapshot copied.
 * @retval -EINVAL Invalid output pointer.
 * @retval -ENOENT The peer has been removed or is unknown.
 */
int wireguard_peer_get_info(int peer_id, struct wireguard_peer_info *info);

/* Bind plaintext to its current owner before network TX queues can retain it. */
void wireguard_prepare_tx(struct net_if *iface, struct net_pkt *pkt);

/** @brief Location of an incoming encrypted WireGuard datagram. */
enum wireguard_path_kind {
	/** Datagram received on a physical UDP path. */
	WIREGUARD_PATH_DIRECT,
	/** Datagram received on an authenticated relay connection. */
	WIREGUARD_PATH_RELAY,
};

/** @brief Receive metadata and a reply path for an alternate transport. */
struct wireguard_datagram_path {
	/** Type of transport used to receive this datagram. */
	enum wireguard_path_kind kind;
	/** Physical interface for a direct path. */
	struct net_if *iface;
	/** Local endpoint for a direct path. */
	struct net_sockaddr_storage local;
	/** Remote endpoint for a direct path. */
	struct net_sockaddr_storage remote;
	/** Opaque transport-supplied identifier for a relay path. */
	uint32_t relay_id;
};

/** @brief Send a WireGuard UDP payload using an alternate transport.
 *
 * The callback must consume or copy the datagram and any reply metadata before
 * returning. It runs in thread context and must not wait for a WireGuard
 * operation, call back into WireGuard, or block on transport I/O.
 *
 * @param peer_id Peer ID associated with this datagram.
 * @param reply_path Incoming path for a handshake reply, or NULL to select a path.
 * @param data WireGuard datagram without outer UDP or IP headers.
 * @param len Number of bytes in @p data.
 * @param user_data Transport context supplied by the caller.
 * @return 0 if accepted for transmission, a negative errno on failure.
 */
typedef int (*wireguard_datagram_send_t)(int peer_id,
					const struct wireguard_datagram_path *reply_path,
					const uint8_t *data, size_t len, void *user_data);

/** @brief Encrypted datagram transport for a peer. */
struct wireguard_transport {
	/** Callback for each outgoing datagram. */
	wireguard_datagram_send_t send;
	/** Caller-owned context kept alive until peer removal succeeds. */
	void *user_data;
};

/** @brief Add a peer that sends and receives datagrams via a caller-owned transport.
 *
 * @param peer_config Peer configuration, as for wireguard_peer_add().
 * @param peer_iface Output virtual interface for the new peer.
 * @param transport Transport with a non-NULL send callback. Copied on success.
 * @return Positive peer ID on success, a negative errno on failure.
 */
int wireguard_peer_add_with_transport(struct wireguard_peer_config *peer_config,
				      struct net_if **peer_iface,
				      const struct wireguard_transport *transport);

/** @brief Add a WireGuard peer to an explicitly selected local identity.
 *
 * Enable CONFIG_WIREGUARD_MULTI_PEER and provision the private key on @p iface
 * before adding the first peer. All peers added this way share that interface
 * and key. Only the longest matching prefix owns an address on this interface.
 * Two different peers cannot own the same prefix. Each local identity that
 * uses the native UDP listener must have a distinct key.
 *
 * Plaintext admitted to the network transmit path is bound to the current
 * peer before it can be queued. Transmission drops the packet if that peer
 * no longer owns the destination.
 *
 * @param iface WireGuard virtual interface for the local identity.
 * @param peer_config Peer configuration.
 * @param transport Alternate transport with a non-NULL send callback, or NULL
 *                  for native UDP. Copied on success.
 * @retval -ENOTSUP Multi-peer mode is disabled.
 * @retval -EINVAL Invalid configuration or missing interface key.
 * @retval -EALREADY This public key is already a peer on the interface.
 * @retval -EADDRINUSE A different peer owns a supplied prefix, or another
 *                     native UDP interface uses the same local key.
 * @return Positive peer ID on success, or another negative errno on failure.
 */
int wireguard_peer_add_to_interface(struct net_if *iface,
				    const struct wireguard_peer_config *peer_config,
				    const struct wireguard_transport *transport);

/** @brief Peer in a complete managed WireGuard peer set. */
struct wireguard_policy_peer {
	/** Stable identity supplied by the policy owner. It must be nonzero. */
	uint64_t identity;
	/** Exclusive deadline in k_uptime_get() milliseconds, or zero for no deadline. */
	int64_t expires_at_ms;
	/** Peer configuration. A nonzero preshared key is unsupported. */
	struct wireguard_peer_config config;
	/** Physical interface for native UDP, or NULL for system route selection. */
	struct net_if *underlay;
	/** Optional caller-owned datagram transport. Copied when the set is published. */
	const struct wireguard_transport *transport;
	/** Optional destination for the installed peer ID. Written only on success. */
	int *peer_id;
};

/** @brief Decide whether authenticated IP plaintext may enter the network stack.
 *
 * The byte range contains the complete inner IP packet without WireGuard
 * padding. The callback runs under the WireGuard mutex and must not block,
 * reenter WireGuard, or retain the byte range.
 *
 * @param identity Identity of the authenticated peer.
 * @param ip Inner IP packet.
 * @param len Inner IP packet length.
 * @param context Context supplied with the installed peer set.
 * @return true to admit the packet.
 */
typedef bool (*wireguard_policy_input_t)(uint64_t identity, const uint8_t *ip, size_t len,
					 void *context);

/** @brief Atomically publish a complete managed peer set on a WireGuard interface.
 *
 * Provision the private key on @p iface before first use.
 * Entries without a transport use native UDP.
 * Peers retaining both their identity and public key retain their session.
 * Each changed identity or key requires an unused peer slot during preparation.
 * CONFIG_WIREGUARD_POLICY_STAGING_PEERS provides such slots at full capacity.
 * The interface remains provisioned and attached for a zero-peer policy.
 *
 * The configuration array is borrowed during the call. On success the old
 * callback context can be released. Keep @p context valid until the next
 * successful replacement or release. Old plaintext deliveries and outer
 * packet handoffs into the network stack have returned by successful cutover.
 * WireGuard-owned queued packets are checked against current membership and
 * route ownership before handoff. A finite deadline excludes the peer from
 * routing and prevents new packet handoffs even if the policy owner has yet
 * to remove that peer. New route selections may use a broader surviving
 * prefix. Queued packets retain their originally selected peer. Packets
 * already handed to the network stack may still be in flight.
 *
 * Call from a thread independent of WireGuard delivery and management
 * callbacks. A failed preflight leaves the installed policy intact.
 *
 * @param iface Managed WireGuard interface.
 * @param peers Complete replacement set, or NULL when @p count is zero.
 * @param count Number of entries in @p peers.
 * @param allow_input Required admission callback, including for an empty set.
 * @param context Context for @p allow_input.
 * @retval 0 Policy installed.
 * @retval -ENOTSUP Managed policy or a requested configuration is unsupported.
 * @retval -EINVAL Invalid configuration or missing local private key.
 * @retval -EALREADY Duplicate peer identity or public key.
 * @retval -EADDRINUSE Duplicate prefix owner or local UDP identity.
 * @retval -ENOSPC Configured peer or staging capacity exceeded.
 * @retval -EBUSY Interface or lifecycle operation already in use.
 * @return Other negative errno on preparation or attachment failure.
 */
int wireguard_replace_peers(struct net_if *iface, const struct wireguard_policy_peer *peers,
			    size_t count, wireguard_policy_input_t allow_input, void *context);

/** @brief Drop the managed peer set, detach the interface and release its key.
 *
 * Call from a thread independent of WireGuard delivery and management
 * callbacks. Successful release ends every use of the admission context.
 * The caller owns any addresses configured on the interface.
 * An unused, unattached interface with a provisioned key can also be released.
 *
 * @param iface Managed WireGuard interface.
 * @retval 0 Policy released.
 * @retval -EINVAL Invalid WireGuard interface.
 * @retval -ENOTSUP Managed policy is disabled.
 * @retval -ENOENT Interface has no managed policy or unused private key.
 * @retval -EBUSY A lifecycle operation is in progress.
 * @return Other negative errno on interface shutdown failure.
 */
int wireguard_release_peers(struct net_if *iface);

/** @brief Find the authenticated identity that owns a managed destination.
 *
 * @param iface Managed WireGuard interface.
 * @param family Destination address family.
 * @param address Destination in network byte order.
 * @return Installed peer identity, or zero if no live route owns the destination.
 */
uint64_t wireguard_policy_route_owner(struct net_if *iface, net_sa_family_t family,
				     const void *address);

/** @brief Immediately stop new route selections and handoffs for a managed policy.
 *
 * Serialize with the policy owner's replacement and release calls. Use this
 * before attempting removal after a loss of authority. The next successful
 * replacement establishes authorization again. In-flight handoffs are
 * drained by replacement or release.
 *
 * @param iface Managed WireGuard interface.
 */
void wireguard_policy_block(struct net_if *iface);

/** @brief Receive a direct UDP datagram for a shared WireGuard identity.
 *
 * Use this entry point for alternate transports that supply direct UDP paths.
 * The core finds the sending peer within @p iface. Relay adapters use
 * wireguard_peer_receive_datagram() with the identity authenticated by the
 * relay connection.
 *
 * @param iface WireGuard virtual interface used to add the peer.
 * @param path Direct path with physical interface and both UDP endpoints.
 * @param data WireGuard datagram without outer UDP or IP headers.
 * @param len Number of bytes in @p data, at most CONFIG_WIREGUARD_BUF_LEN.
 * @retval 0 Datagram accepted.
 * @retval -EINVAL Invalid arguments or relay path.
 * @retval -ENOTSUP Multi-peer mode is disabled.
 * @return Other negative errno on lookup, authentication, or delivery failure.
 */
int wireguard_interface_receive_datagram(struct net_if *iface,
					 const struct wireguard_datagram_path *path,
					 const uint8_t *data, size_t len);

/** @brief Process an encrypted WireGuard datagram on a transport-backed peer.
 *
 * Call in thread context. Relay adapters must authenticate the relay connection
 * and map the relay's claimed WireGuard peer key to @p peer_id before delivery.
 * Direct paths require an interface and two UDP endpoints of the same address
 * family with nonzero ports. Relay packets leave the learned UDP endpoint intact.
 * Data and path are borrowed for the duration of this call. A stale peer ID
 * cannot deliver to a replacement peer.
 *
 * @param peer_id Installed peer ID from an add or managed-set operation with an alternate
 *                transport.
 * @param path Valid direct or relay receive metadata.
 * @param data WireGuard datagram without outer UDP or IP headers.
 * @param len Number of bytes in @p data, at most CONFIG_WIREGUARD_BUF_LEN.
 * @retval 0 Datagram accepted.
 * @retval -ENOENT No such peer or matching session.
 * @retval -EACCES The peer is blocked by policy or expired.
 * @retval -ENOTSUP Peer uses the native UDP transport.
 * @retval -EMSGSIZE Invalid datagram size.
 * @return Other negative errno on authentication, resource, or delivery failure.
 */
int wireguard_peer_receive_datagram(int peer_id, const struct wireguard_datagram_path *path,
				    const uint8_t *data, size_t len);

#endif /* ZEPHYR_SUBSYS_NET_LIB_WIREGUARD_WG_PRIVATE_H_ */
