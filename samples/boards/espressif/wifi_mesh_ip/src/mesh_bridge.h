/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESH_BRIDGE_H
#define MESH_BRIDGE_H

#include <zephyr/net/net_if.h>

/**
 * @brief Bring up the root-side IP bridge.
 *
 * Runs on the node elected root. Starts the DHCP server that leases addresses
 * to the mesh sub-network over the mesh interface, and installs the NAT rules
 * that masquerade child traffic out the station uplink so the whole mesh
 * reaches the external network through the root.
 *
 * @param sta_iface Station interface providing the external uplink.
 */
void mesh_bridge_setup_root(struct net_if *sta_iface);

/**
 * @brief Bring up the child-side IP bridge.
 *
 * Runs on a non-root node. Starts a DHCP client on the mesh interface to
 * obtain an address from the root's DHCP server over the mesh transport,
 * giving the node IP connectivity through the root.
 */
void mesh_bridge_setup_child(void);

#endif /* MESH_BRIDGE_H */
