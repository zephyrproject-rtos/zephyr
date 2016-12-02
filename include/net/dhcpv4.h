/** @file
 *  @brief DHCPv4 Client Handler
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DHCPV4_H
#define __DHCPV4_H

#include <misc/slist.h>
#include <stdint.h>

/** Current state of DHCPv4 client address negotiation */
enum net_dhcpv4_state {
	NET_DHCPV4_INIT,
	NET_DHCPV4_DISCOVER,
	NET_DHCPV4_OFFER,
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

#endif /* __DHCPV4_H */
