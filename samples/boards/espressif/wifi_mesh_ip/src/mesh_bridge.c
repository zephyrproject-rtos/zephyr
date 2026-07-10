/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * IP-over-mesh policy for the sample: address the mesh interface, drive DHCP,
 * NAT and routing on the root, and request an address on a child. The
 * Ethernet-over-mesh transport itself is provided by the Wi-Fi driver
 * (esp_wifi_mesh_netif_get()); this file only wires the IP layer on top.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/ipv4_nat.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

#include "mesh_bridge.h"

LOG_MODULE_DECLARE(wifi_mesh_ip, LOG_LEVEL_INF);

static struct net_mgmt_event_callback mesh_dhcp_cb;
static struct net_mgmt_event_callback mesh_uplink_cb;

/* Root serves this private subnet to the mesh; .1 is the root itself. */
#define MESH_SUBNET_A 192
#define MESH_SUBNET_B 168
#define MESH_SUBNET_C 4

/*
 * When the root station gets its uplink lease, register the router as the
 * default router and announce toDS reachability so the mesh starts admitting
 * child traffic bound for the external network; before the uplink is up there
 * is nowhere to forward it.
 */
static void mesh_uplink_bound(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		struct net_in_addr gw = net_if_ipv4_get_gw(iface);

		net_if_ipv4_router_add(iface, &gw, true, 0);
		esp_wifi_mesh_set_tods_reachable(true);
	}
}

static void mesh_dhcp_bound(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			    struct net_if *iface)
{
	static atomic_t bound_once;

	ARG_UNUSED(cb);

	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	/*
	 * DHCP raises BOUND on the initial lease and again on every renewal.
	 * The gateway and route only need setting once, so run this block just
	 * once and ignore later renewals.
	 */
	if (atomic_set(&bound_once, 1) != 0) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];
	struct net_if_addr *ifaddr = &iface->config.ip.ipv4->unicast[0].ipv4;

	LOG_INF("child got IP %s over mesh",
		net_addr_ntop(AF_INET, &ifaddr->address.in_addr, buf, sizeof(buf)));

	/* Route off-subnet traffic to the root over the mesh. */
	struct net_in_addr gw = {{{MESH_SUBNET_A, MESH_SUBNET_B, MESH_SUBNET_C, 1}}};

	net_if_ipv4_set_gw(esp_wifi_mesh_netif_get(), &gw);
	net_if_ipv4_router_add(esp_wifi_mesh_netif_get(), &gw, true, 0);
}

/*
 * Root: own the mesh subnet gateway address, serve DHCP to the children, and
 * masquerade their traffic out the station interface toward the router. The
 * Zephyr network stack does the DHCP, NAT and forwarding; only the addresses
 * and the rule are set here.
 */
#if defined(CONFIG_NET_DHCPV4_SERVER) && defined(CONFIG_NET_IPV4_NAT)
void mesh_bridge_setup_root(struct net_if *sta_iface)
{
	struct net_if *mesh_iface = esp_wifi_mesh_netif_get();
	struct net_in_addr gw = {{{MESH_SUBNET_A, MESH_SUBNET_B, MESH_SUBNET_C, 1}}};
	struct net_in_addr netmask = {{{255, 255, 255, 0}}};
	struct net_in_addr pool_start = {{{MESH_SUBNET_A, MESH_SUBNET_B, MESH_SUBNET_C, 2}}};

	/*
	 * The mesh stack drives the station to the router, bypassing the normal
	 * connect path that would bring the station interface up. Force it up so
	 * the network stack can obtain the uplink address and route.
	 */
	esp_wifi_mesh_bind_sta_rx();
	net_mgmt_init_event_callback(&mesh_uplink_cb, mesh_uplink_bound, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&mesh_uplink_cb);

	net_if_carrier_on(sta_iface);
	net_if_dormant_off(sta_iface);
	net_dhcpv4_start(sta_iface);

	net_if_ipv4_addr_add(mesh_iface, &gw, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(mesh_iface, &gw, &netmask);

	/*
	 * Add an explicit connected route for the mesh subnet over the mesh
	 * interface. When IPv4 forwarding is enabled the forwarding lookup does
	 * not resolve on-link subnets on its own, so a reply masqueraded back to
	 * a child would otherwise follow the default route out the station. The
	 * route carries no next hop, which makes the stack resolve the child
	 * address directly on the mesh interface.
	 */
	struct net_in_addr subnet = {{{MESH_SUBNET_A, MESH_SUBNET_B, MESH_SUBNET_C, 0}}};

	if (net_if_ipv4_route_add(mesh_iface, &subnet, 24, NULL, UINT32_MAX) < 0) {
		LOG_ERR("mesh subnet route add failed");
	}

	if (net_dhcpv4_server_start(mesh_iface, &pool_start) < 0) {
		LOG_ERR("mesh DHCP server start failed");
	}

	/* NAT is per protocol, so masquerade TCP, UDP and ICMP. */
	static const enum net_ip_protocol protos[] = {
		NET_IPPROTO_TCP,
		NET_IPPROTO_UDP,
		NET_IPPROTO_ICMP,
	};

	for (size_t i = 0; i < ARRAY_SIZE(protos); i++) {
		struct net_iptable_rule_params rule = {0};

		rule.input_iface_idx = net_if_get_by_iface(mesh_iface);
		rule.output_iface_idx = net_if_get_by_iface(sta_iface);
		memcpy(rule.src, (uint8_t[]){MESH_SUBNET_A, MESH_SUBNET_B, MESH_SUBNET_C, 0}, 4);
		memcpy(rule.src_mask, (uint8_t[]){255, 255, 255, 0}, 4);
		rule.proto = protos[i];
		rule.priority = 5;

		if (net_ipv4_table_rule_add(&rule) < 0) {
			LOG_ERR("mesh NAT rule add failed (proto %d)", protos[i]);
		}
	}

	LOG_INF("mesh root IP: gateway %d.%d.%d.1, DHCP + NAT up", MESH_SUBNET_A, MESH_SUBNET_B,
		MESH_SUBNET_C);
}
#else
/*
 * Child-only build: the DHCP server and NAT the root relay needs are disabled,
 * so a node built this way cannot serve the mesh subnet. Useful on a SoC whose
 * RAM does not fit the root configuration.
 */
void mesh_bridge_setup_root(struct net_if *sta_iface)
{
	ARG_UNUSED(sta_iface);

	LOG_ERR("elected root, but this image is built without the DHCP server "
		"and NAT the root needs (CONFIG_NET_DHCPV4_SERVER, "
		"CONFIG_NET_IPV4_NAT); the mesh subnet has no gateway");
}
#endif /* CONFIG_NET_DHCPV4_SERVER && CONFIG_NET_IPV4_NAT */

/* Child: get an address from the root's DHCP server over the mesh. */
void mesh_bridge_setup_child(void)
{
	net_mgmt_init_event_callback(&mesh_dhcp_cb, mesh_dhcp_bound, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&mesh_dhcp_cb);

	net_dhcpv4_start(esp_wifi_mesh_netif_get());
	LOG_INF("mesh child IP: requesting address over mesh");
}
