/** @file
 *  @brief DHCPv4 Client Handler
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DHCPV4_H
#define __DHCPV4_H

/**
 * @brief DHCPv4
 * @defgroup dhcpv4 DHCPv4
 * @{
 */

#include <misc/slist.h>
#include <stdint.h>

/** Current state of DHCPv4 client address negotiation.
 *
 * Additions removals and reorders in this definition must be
 * reflected within corresponding changes to net_dhcpv4_state_name.
 */
enum net_dhcpv4_state {
	NET_DHCPV4_INIT,
	NET_DHCPV4_DISCOVER,
	NET_DHCPV4_REQUEST,
	NET_DHCPV4_RENEWAL,
	NET_DHCPV4_ACK,
};

/**
 *  @brief Start DHCPv4 client
 *
 *  @details Start DHCPv4 client on a given interface. DHCPv4 client
 *  will start negotiation for IPv4 address. Once the negotiation is
 *  success IPv4 address details will be added to interface.
 *
 *  @param iface A valid pointer on an interface
 */
void net_dhcpv4_start(struct net_if *iface);

/**
 * @}
 */

#endif /* __DHCPV4_H */
