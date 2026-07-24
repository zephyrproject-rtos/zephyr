/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief DHCPv6 server (prefix delegation), RFC 8415
 */

#ifndef ZEPHYR_INCLUDE_NET_DHCPV6_SERVER_H_
#define ZEPHYR_INCLUDE_NET_DHCPV6_SERVER_H_

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHCPv6 server
 * @defgroup dhcpv6_server DHCPv6 server
 * @since 4.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

struct net_if;

/** @brief DHCPv6 server configuration parameters. */
struct net_dhcpv6_server_params {
	/** Base of the prefix pool used for prefix delegation (IA_PD). Prefixes
	 *  handed to requesting routers are derived from this base by varying
	 *  the subnet bits.
	 */
	struct net_in6_addr prefix;

	/** Length (in bits) of the prefixes delegated to clients, e.g. 64. */
	uint8_t prefix_len;

	/** Base of the address pool used for non-temporary address assignment
	 *  (IA_NA). Leased addresses are derived from this base.
	 */
	struct net_in6_addr addr;

	/** Hand out IA_NA (non-temporary) addresses. */
	bool offer_addr : 1;

	/** Hand out IA_PD (delegated) prefixes. */
	bool offer_prefix : 1;

	/** Valid lifetime advertised to clients (seconds). 0 selects a default. */
	uint32_t valid_lifetime;

	/** Preferred lifetime advertised to clients (seconds). 0 selects a
	 *  default.
	 */
	uint32_t preferred_lifetime;
};

/**
 * @brief Start the DHCPv6 server on the given interface.
 *
 * @param iface A valid pointer to a network interface
 * @param params Server configuration parameters (copied)
 *
 * @return 0 on success, negative errno otherwise.
 */
int net_dhcpv6_server_start(struct net_if *iface,
			    struct net_dhcpv6_server_params *params);

/**
 * @brief Stop the DHCPv6 server on the given interface.
 *
 * @param iface A valid pointer to a network interface
 *
 * @return 0 on success, negative errno otherwise.
 */
int net_dhcpv6_server_stop(struct net_if *iface);

/** @cond INTERNAL_HIDDEN */

/** @brief A DHCPv6 server lease (binding), as passed to the foreach callback. */
struct net_dhcpv6_server_lease {
	/** Client DUID (raw bytes). */
	const uint8_t *duid;
	/** Length of @ref duid in bytes. */
	uint8_t duid_len;
	/** True if an IA_NA address is assigned to the client. */
	bool has_addr;
	/** Assigned IA_NA address (valid when @ref has_addr is true). */
	struct net_in6_addr addr;
	/** True if an IA_PD prefix is delegated to the client. */
	bool has_prefix;
	/** Delegated IA_PD prefix (valid when @ref has_prefix is true). */
	struct net_in6_addr prefix;
	/** Length in bits of the delegated prefix. */
	uint8_t prefix_len;
	/** Absolute lease expiry as a k_uptime_get() timestamp (ms), or 0 if
	 *  the binding is not yet bound.
	 */
	int64_t expiry;
};

/** @endcond */

/**
 * @typedef net_dhcpv6_server_lease_cb_t
 * @brief Callback used while iterating over DHCPv6 server leases.
 *
 * @param iface Pointer to the network interface the lease belongs to
 * @param lease Pointer to the DHCPv6 server lease
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_dhcpv6_server_lease_cb_t)(
	struct net_if *iface, struct net_dhcpv6_server_lease *lease,
	void *user_data);

/**
 * @brief Iterate over all DHCPv6 server leases and call @p cb for each.
 *
 * If @p iface is NULL, iterate over the lease of any running server instance,
 * otherwise only over the server running on @p iface.
 *
 * @param iface Pointer to the network interface, can be NULL
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 *
 * @return 0 on success, -ENOENT if no server is running on @p iface, negative
 *         errno otherwise.
 */
int net_dhcpv6_server_foreach_lease(struct net_if *iface,
				    net_dhcpv6_server_lease_cb_t cb,
				    void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DHCPV6_SERVER_H_ */
