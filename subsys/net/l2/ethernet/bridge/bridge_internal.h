/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Ethernet bridge internal functions
 *
 * Shared between the bridge sources. Not part of the public API.
 */

#ifndef ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_BRIDGE_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_BRIDGE_INTERNAL_H_

#include <stdbool.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet_bridge.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the port state of a bridged interface
 *
 * @param iface Bridged Ethernet interface
 *
 * @return The state of the port of @p iface, or
 *         ETH_BRIDGE_PORT_STATE_DISCARDING if @p iface is not bridged.
 */
enum eth_bridge_port_state eth_bridge_port_get_state(struct net_if *iface);

/**
 * @brief Set the port state of a bridged interface
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param state New port state
 *
 * @retval 0 The state was set.
 * @retval -EINVAL The state is not a valid port state.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_port_set_state(struct net_if *br, struct net_if *iface,
			      enum eth_bridge_port_state state);

/**
 * @brief Check whether a bridged interface forwards frames
 *
 * @param iface Bridged Ethernet interface
 *
 * @return true if the port of @p iface is forwarding, false otherwise.
 */
static inline bool eth_bridge_port_is_forwarding(struct net_if *iface)
{
	return eth_bridge_port_get_state(iface) == ETH_BRIDGE_PORT_STATE_FORWARDING;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_BRIDGE_INTERNAL_H_ */
