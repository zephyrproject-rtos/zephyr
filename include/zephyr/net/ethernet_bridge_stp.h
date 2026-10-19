/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Rapid Spanning Tree Protocol for Ethernet bridges
 */

#ifndef ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_STP_H_
#define ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_STP_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rapid Spanning Tree Protocol for Ethernet bridges
 * @defgroup eth_bridge_stp Ethernet Bridge Spanning Tree API
 * @since 4.6
 * @version 0.1.0
 * @ingroup eth_bridge
 * @{
 */

/** Port Role (IEEE Std 802.1Q-2014 13.27.66) */
enum eth_bridge_stp_role {
	/** The port is disabled, or the protocol is off */
	ETH_BRIDGE_STP_ROLE_DISABLED,
	/** The port is the best path to the root bridge */
	ETH_BRIDGE_STP_ROLE_ROOT,
	/** The port connects its LAN to the root bridge */
	ETH_BRIDGE_STP_ROLE_DESIGNATED,
	/** An alternate path to the root bridge, blocked */
	ETH_BRIDGE_STP_ROLE_ALTERNATE,
	/** A backup path to a LAN another port of this bridge connects, blocked */
	ETH_BRIDGE_STP_ROLE_BACKUP,
};

/** Force Protocol Version (IEEE Std 802.1Q-2014 13.7.2) */
enum eth_bridge_stp_version {
	/** STP compatible: STP BPDUs and no rapid transitions */
	ETH_BRIDGE_STP_VERSION_STP = 0,
	/** RSTP, the default */
	ETH_BRIDGE_STP_VERSION_RSTP = 2,
};

/** Point-to-point setting of a port (IEEE Std 802.1Q-2014 13.28.16) */
enum eth_bridge_stp_p2p {
	/** Point-to-point if the link is full duplex */
	ETH_BRIDGE_STP_P2P_AUTO,
	/** The port is attached to a point-to-point link */
	ETH_BRIDGE_STP_P2P_ON,
	/** The port is attached to a shared LAN */
	ETH_BRIDGE_STP_P2P_OFF,
};

/** Spanning tree state of a bridge */
struct eth_bridge_stp_status {
	/** The protocol runs on the bridge */
	bool enabled;
	/** Force Protocol Version */
	enum eth_bridge_stp_version version;
	/** Bridge Identifier of this bridge */
	uint64_t bridge_id;
	/** Bridge Identifier of the root bridge */
	uint64_t root_id;
	/** Cost of the path to the root bridge */
	uint32_t root_path_cost;
	/** Root Port, NULL if this bridge is the root */
	struct net_if *root_port;
	/** Max Age in use, in seconds */
	uint8_t max_age;
	/** Forward Delay in use, in seconds */
	uint8_t forward_delay;
	/** Bridge Max Age, in seconds */
	uint8_t bridge_max_age;
	/** Bridge Forward Delay, in seconds */
	uint8_t bridge_forward_delay;
	/** Transmit Hold Count */
	uint8_t tx_hold_count;
	/** A topology change is being propagated (IEEE Std 802.1Q-2014 12.8.1.1.3) */
	bool topology_change;
	/** Topology changes since the bridge was initialized */
	uint32_t topology_change_count;
	/** Seconds since the last topology change */
	uint32_t time_since_topology_change;
};

/** Spanning tree state of a bridge port */
struct eth_bridge_stp_port_status {
	/** Port Role */
	enum eth_bridge_stp_role role;
	/** The port learns source addresses */
	bool learning;
	/** The port forwards frames */
	bool forwarding;
	/** The port is an Edge Port */
	bool oper_edge;
	/** AdminEdge setting, see eth_bridge_stp_port_set_edge() */
	bool admin_edge;
	/** AutoEdge setting, see eth_bridge_stp_port_set_edge() */
	bool auto_edge;
	/** The port is attached to a point-to-point link */
	bool oper_p2p;
	/** Point-to-point setting, see eth_bridge_stp_port_set_p2p() */
	enum eth_bridge_stp_p2p p2p;
	/** The Port Path Cost is derived from the link speed */
	bool path_cost_auto;
	/** The port sends RST BPDUs, false when it sends STP BPDUs */
	bool send_rstp;
	/** Port Identifier: priority in the 4 most significant bits, then the port number */
	uint16_t port_id;
	/** Port Path Cost in use */
	uint32_t path_cost;
	/** Root bridge known on the port's LAN */
	uint64_t designated_root;
	/** Designated Bridge of the port's LAN */
	uint64_t designated_bridge;
	/** Designated Port of the port's LAN */
	uint16_t designated_port;
	/** Root path cost of the Designated Bridge */
	uint32_t designated_cost;
};

/**
 * @brief Information of the NET_EVENT_ETHERNET_BRIDGE_PORT_STATE event
 *
 * The event is raised on the port interface when the role, learning or
 * forwarding state of the port changes, also when the protocol starts or
 * stops on the bridge.
 */
struct eth_bridge_stp_port_event {
	/** Bridge interface of the port */
	struct net_if *bridge;
	/** Port Role */
	enum eth_bridge_stp_role role;
	/** The port learns source addresses */
	bool learning;
	/** The port forwards frames */
	bool forwarding;
};

/**
 * @brief Information of the NET_EVENT_ETHERNET_BRIDGE_NEW_ROOT event
 *
 * The event is raised on the bridge interface when the root bridge changes,
 * also when the protocol starts and the bridge is its own root.
 */
struct eth_bridge_stp_root_event {
	/** Bridge Identifier of the root bridge */
	uint64_t root_id;
	/** Cost of the path to the root bridge */
	uint32_t root_path_cost;
	/** Root Port, NULL if this bridge is the root */
	struct net_if *root_port;
};

/**
 * @brief Information of the NET_EVENT_ETHERNET_BRIDGE_TOPOLOGY_CHANGE event
 *
 * The event is raised on the bridge interface when a topology change starts:
 * a port starts propagating one while none did. The learned FDB entries of
 * the other ports are then removed.
 */
struct eth_bridge_stp_topology_event {
	/** Topology changes since the bridge was initialized, this one included */
	uint32_t topology_change_count;
};

/**
 * @brief Start or stop the spanning tree protocol on a bridge
 *
 * The protocol is off by default and the bridge then forwards on every port.
 * When started, every port discards frames until the protocol has decided
 * its role. When stopped, every port forwards again and the learned FDB
 * entries of the bridge are removed.
 *
 * The Bridge Identifier is made of the Bridge Priority and the MAC address
 * of the bridge interface. Use a stable bridge MAC address, as a random one
 * changes the Bridge Identifier on every boot.
 *
 * @param br Bridge interface
 * @param enable true to start, false to stop
 *
 * @retval 0 The protocol was started or stopped.
 * @retval -EALREADY The protocol was already in that state.
 * @retval -EINVAL @p br is not a bridge interface.
 */
int eth_bridge_stp_enable(struct net_if *br, bool enable);

/**
 * @brief Get the spanning tree state of a bridge
 *
 * @param br Bridge interface
 * @param status Filled with the state of @p br
 *
 * @retval 0 The status was filled.
 * @retval -EINVAL @p br is not a bridge interface.
 */
int eth_bridge_stp_get_status(struct net_if *br, struct eth_bridge_stp_status *status);

/**
 * @brief Get the spanning tree state of a bridge port
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param status Filled with the state of the port of @p iface
 *
 * @retval 0 The status was filled.
 * @retval -EINVAL @p br is not a bridge interface.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_get_status(struct net_if *br, struct net_if *iface,
				   struct eth_bridge_stp_port_status *status);

/**
 * @brief Set the Bridge Priority
 *
 * The bridge with the best (numerically lowest) priority becomes the root.
 *
 * @param br Bridge interface
 * @param priority 0 to 61440 in steps of 4096, 32768 by default
 *
 * @retval 0 The priority was set.
 * @retval -EINVAL @p br is not a bridge interface or @p priority is not valid.
 */
int eth_bridge_stp_set_bridge_priority(struct net_if *br, uint16_t priority);

/**
 * @brief Set the Bridge Max Age and Bridge Forward Delay
 *
 * The values must satisfy 2 * (forward_delay - 1) >= max_age >= 6.
 *
 * @param br Bridge interface
 * @param max_age Bridge Max Age, 6 to 40 seconds, 20 by default
 * @param forward_delay Bridge Forward Delay, 4 to 30 seconds, 15 by default
 *
 * @retval 0 The times were set.
 * @retval -EINVAL @p br is not a bridge interface or a value is not valid.
 */
int eth_bridge_stp_set_times(struct net_if *br, uint8_t max_age, uint8_t forward_delay);

/**
 * @brief Set the Transmit Hold Count
 *
 * @param br Bridge interface
 * @param count Most BPDUs a port sends in one second, 1 to 10, 6 by default
 *
 * @retval 0 The count was set.
 * @retval -EINVAL @p br is not a bridge interface or @p count is not valid.
 */
int eth_bridge_stp_set_tx_hold_count(struct net_if *br, uint8_t count);

/**
 * @brief Set the Force Protocol Version
 *
 * Restarts the protocol on all ports of a running bridge.
 *
 * @param br Bridge interface
 * @param version Protocol version
 *
 * @retval 0 The version was set.
 * @retval -EINVAL @p br is not a bridge interface or @p version is not valid.
 */
int eth_bridge_stp_set_version(struct net_if *br, enum eth_bridge_stp_version version);

/**
 * @brief Set the priority of a bridge port
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param priority 0 to 240 in steps of 16, 128 by default
 *
 * @retval 0 The priority was set.
 * @retval -EINVAL @p br is not a bridge interface or @p priority is not valid.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_set_priority(struct net_if *br, struct net_if *iface, uint8_t priority);

/**
 * @brief Set the Port Path Cost of a bridge port
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param cost 1 to 200000000, or 0 to derive it from the link speed
 *             (IEEE Std 802.1Q-2014 Table 13-4), the default
 *
 * @retval 0 The cost was set.
 * @retval -EINVAL @p br is not a bridge interface or @p cost is not valid.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_set_path_cost(struct net_if *br, struct net_if *iface, uint32_t cost);

/**
 * @brief Set how a bridge port detects that it is an Edge Port
 *
 * An Edge Port has no bridge behind it and forwards without delay.
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param admin_edge The port is an Edge Port from the start, false by default
 * @param auto_edge The port becomes an Edge Port when it receives no BPDU,
 *                  true by default
 *
 * @retval 0 The setting was stored.
 * @retval -EINVAL @p br is not a bridge interface.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_set_edge(struct net_if *br, struct net_if *iface, bool admin_edge,
				 bool auto_edge);

/**
 * @brief Set whether a bridge port is attached to a point-to-point link
 *
 * Rapid transitions to forwarding need a point-to-point link.
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 * @param p2p Point-to-point setting, ETH_BRIDGE_STP_P2P_AUTO by default
 *
 * @retval 0 The setting was stored.
 * @retval -EINVAL @p br is not a bridge interface or @p p2p is not valid.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_set_p2p(struct net_if *br, struct net_if *iface,
				enum eth_bridge_stp_p2p p2p);

/**
 * @brief Make a bridge port check whether STP bridges are still attached
 *
 * A port that has heard an STP bridge keeps sending STP BPDUs. This makes
 * it send RST BPDUs again, and fall back once more if an STP bridge is
 * still there.
 *
 * @param br Bridge interface
 * @param iface Ethernet interface bridged by @p br
 *
 * @retval 0 The check was started.
 * @retval -EINVAL @p br is not a bridge interface.
 * @retval -ENOENT @p iface is not a port of @p br.
 */
int eth_bridge_stp_port_mcheck(struct net_if *br, struct net_if *iface);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_STP_H_ */
