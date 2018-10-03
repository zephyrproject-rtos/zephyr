/** @file
 *  @brief DHCPv4 Client Handler
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DHCPV4_H_
#define ZEPHYR_INCLUDE_NET_DHCPV4_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHCPv4
 * @defgroup dhcpv4 DHCPv4
 * @ingroup networking
 * @{
 */

#include <misc/slist.h>
#include <zephyr/types.h>

/** Current state of DHCPv4 client address negotiation.
 *
 * Additions removals and reorders in this definition must be
 * reflected within corresponding changes to net_dhcpv4_state_name.
 */
enum net_dhcpv4_state {
	NET_DHCPV4_DISABLED,
	NET_DHCPV4_INIT,
	NET_DHCPV4_SELECTING,
	NET_DHCPV4_REQUESTING,
	NET_DHCPV4_RENEWING,
	NET_DHCPV4_REBINDING,
	NET_DHCPV4_BOUND,
} __packed;

/**
 *  @brief Start DHCPv4 client on an iface
 *
 *  @details Start DHCPv4 client on a given interface. DHCPv4 client
 *  will start negotiation for IPv4 address. Once the negotiation is
 *  success IPv4 address details will be added to interface.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_start(struct net_if *iface);

/**
 *  @brief Stop DHCPv4 client on an iface
 *
 *  @details Stop DHCPv4 client on a given interface. DHCPv4 client
 *  will remove all configuration obtained from a DHCP server from the
 *  interface and stop any further negotiation with the server.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_stop(struct net_if *iface);

/**
 *  @brief DHCPv4 state name
 *
 *  @internal
 */
const char *net_dhcpv4_state_name(enum net_dhcpv4_state state);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DHCPV4_H_ */
