/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/virtual_mgmt.h>

/* User data for the interface callback */
struct ud {
	struct net_if *tunnel;
	struct net_if *peer;
};

bool is_tunnel(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		return true;
	}

	return false;
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (!ud->tunnel && is_tunnel(iface)) {
		ud->tunnel = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface, const char *ipaddr)
{
	struct net_if_addr *ifaddr;
	struct sockaddr addr;

	if (!net_ipaddr_parse(ipaddr, strlen(ipaddr), &addr)) {
		LOG_ERR("Tunnel peer address \"%s\" invalid.", ipaddr);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.sa_family == AF_INET6) {
		ifaddr = net_if_ipv6_addr_add(iface,
					      &net_sin6(&addr)->sin6_addr,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %d",
				ipaddr, net_if_get_by_iface(iface));
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr.sa_family == AF_INET) {
		ifaddr = net_if_ipv4_addr_add(iface,
					      &net_sin(&addr)->sin_addr,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %d",
				ipaddr, net_if_get_by_iface(iface));
			return -EINVAL;
		}
	}

	return 0;
}

int init_tunnel(void)
{
	struct virtual_interface_req_params params = { 0 };
	struct sockaddr peer = { 0 };
	struct ud ud;
	int ret;
	int mtu;

	memset(&ud, 0, sizeof(ud));

	if (CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR[0] == '\0') {
		LOG_INF("Tunnel peer address not set.");
		return 0;
	}

	if (!net_ipaddr_parse(CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR,
			      strlen(CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR),
			      &peer)) {
		LOG_ERR("Tunnel peer address \"%s\" invalid.",
			CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && peer.sa_family == AF_INET6) {
		struct net_if *iface;

		iface = net_if_ipv6_select_src_iface(
					&net_sin6(&peer)->sin6_addr);
		ud.peer = iface;
		params.family = AF_INET6;
		net_ipaddr_copy(&params.peer6addr,
				&net_sin6(&peer)->sin6_addr);
		mtu = NET_ETH_MTU - sizeof(struct net_ipv6_hdr);

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && peer.sa_family == AF_INET) {
		struct net_if *iface;

		iface = net_if_ipv4_select_src_iface(
					&net_sin(&peer)->sin_addr);
		ud.peer = iface;
		params.family = AF_INET;
		net_ipaddr_copy(&params.peer4addr,
				&net_sin(&peer)->sin_addr);
		mtu = NET_ETH_MTU - sizeof(struct net_ipv4_hdr);

	} else {
		LOG_ERR("Invalid address family %d", peer.sa_family);
		return -EINVAL;
	}

	if (ud.peer == NULL) {
		LOG_ERR("Peer address %s unreachable",
			CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR);
		return -ENETUNREACH;
	}

	net_if_foreach(iface_cb, &ud);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
		       ud.tunnel, &params, sizeof(params));
	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Cannot set peer address %s to "
			"interface %d (%d)",
			CONFIG_NET_SAMPLE_TUNNEL_PEER_ADDR,
			net_if_get_by_iface(ud.tunnel),
			ret);
	}

	params.mtu = mtu;

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
		       ud.tunnel, &params, sizeof(params));
	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Cannot set interface %d MTU to %d (%d)",
			net_if_get_by_iface(ud.tunnel), params.mtu, ret);
	}

	ret = setup_iface(ud.tunnel,
			  CONFIG_NET_SAMPLE_TUNNEL_MY_ADDR);
	if (ret < 0) {
		LOG_ERR("Cannot set IP address %s to tunnel interface",
			CONFIG_NET_SAMPLE_TUNNEL_MY_ADDR);
		return -EINVAL;
	}

	return 0;
}
