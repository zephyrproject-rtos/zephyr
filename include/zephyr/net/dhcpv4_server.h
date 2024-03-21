/** @file
 *  @brief DHCPv4 Server API
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DHCPV4_SERVER_H_
#define ZEPHYR_INCLUDE_NET_DHCPV4_SERVER_H_

#include <zephyr/net/net_ip.h>
#include <zephyr/sys_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHCPv4 server
 * @defgroup dhcpv4_server DHCPv4 server
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct net_if;

#define DHCPV4_CLIENT_ID_MAX_SIZE 20

enum dhcpv4_server_addr_state {
	DHCPV4_SERVER_ADDR_FREE,
	DHCPV4_SERVER_ADDR_RESERVED,
	DHCPV4_SERVER_ADDR_ALLOCATED,
	DHCPV4_SERVER_ADDR_DECLINED,
};

struct dhcpv4_client_id {
	uint8_t buf[DHCPV4_CLIENT_ID_MAX_SIZE];
	uint8_t len;
};

struct dhcpv4_addr_slot {
	enum dhcpv4_server_addr_state state;
	struct dhcpv4_client_id client_id;
	struct in_addr addr;
	uint32_t lease_time;
	k_timepoint_t expiry;
};

/** @endcond */

/**
 *  @brief Start DHCPv4 server instance on an iface
 *
 *  @details Start DHCPv4 server on a given interface. The server will start
 *  listening for DHCPv4 Discover/Request messages on the interface and assign
 *  IPv4 addresses from the configured address pool accordingly.
 *
 *  @param iface A valid pointer on an interface
 *  @param base_addr First IPv4 address from the DHCPv4 address pool. The number
 *  of addresses in the pool is configured statically with Kconfig
 *  (CONFIG_NET_DHCPV4_SERVER_ADDR_COUNT).
 *
 *  @return 0 on success, a negative error code otherwise.
 */
int net_dhcpv4_server_start(struct net_if *iface, struct in_addr *base_addr);

/**
 *  @brief Stop DHCPv4 server instance on an iface
 *
 *  @details Stop DHCPv4 server on a given interface. DHCPv4 requests will no
 *  longer be handled on the interface, and all of the allocations are cleared.
 *
 *  @param iface A valid pointer on an interface
 *
 *  @return 0 on success, a negative error code otherwise.
 */
int net_dhcpv4_server_stop(struct net_if *iface);

/**
 * @typedef net_dhcpv4_lease_cb_t
 * @brief Callback used while iterating over active DHCPv4 address leases
 *
 * @param iface Pointer to the network interface
 * @param lease Pointer to the DHPCv4 address lease slot
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_dhcpv4_lease_cb_t)(struct net_if *iface,
				      struct dhcpv4_addr_slot *lease,
				      void *user_data);

/**
 * @brief Iterate over all DHCPv4 address leases on a given network interface
 * and call callback for each lease. In case no network interface is provided
 * (NULL interface pointer), will iterate over all interfaces running DHCPv4
 * server instance.
 *
 * @param iface Pointer to the network interface, can be NULL
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
int net_dhcpv4_server_foreach_lease(struct net_if *iface,
				    net_dhcpv4_lease_cb_t cb,
				    void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DHCPV4_SERVER_H_ */
