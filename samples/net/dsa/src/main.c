/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dsa_sample, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/dsa.h>

/* User data for the interface callback */
struct ud {
	struct net_if *lan1;
	struct net_if *lan2;
	struct net_if *lan3;
	struct net_if *master;
};

static void dsa_slave_port_setup(struct net_if *iface, struct ud *ud)
{
	struct dsa_context *context = net_if_get_device(iface)->driver_data;

	if (ud->lan1 == NULL) {
		ud->lan1 = context->iface_slave[1];
	}

	if (ud->lan2 == NULL) {
		ud->lan2 = context->iface_slave[2];
	}

	if (ud->lan3 == NULL) {
		ud->lan3 = context->iface_slave[3];
	}
}

static void iface_cb(struct net_if *iface, void *user_data)
{

	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
		if (ud->master == NULL) {
			ud->master = iface;
			return;
		}
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_SLAVE_PORT) {
		dsa_slave_port_setup(iface, ud);
	}
}

static int setup_iface(struct net_if *iface, const char *ipv4_addr, u8_t port)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	int ret;

	ret = dsa_enable_port(iface, port);
	if (ret < 0) {
		LOG_ERR("Cannot enable DSA port %d (%d)", port, ret);
	}

	if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
		LOG_ERR("Invalid address: %s", ipv4_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
		return -EINVAL;
	}

	LOG_DBG("Interface %p port %d setup done.", iface, port);

	return 0;
}

static enum net_verdict dsa_processing_cb(struct net_if *iface,
					  struct net_pkt *pkt)
{
	return NET_DROP;
}

static struct ud ud;
static int init_dsa_ports(void)
{
	int ret;

	(void)memset(&ud, 0, sizeof(ud));
	net_if_foreach(iface_cb, &ud);

	ret = setup_iface(ud.lan1, "192.168.0.3", 1);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(ud.lan2, "192.168.0.4", 2);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(ud.lan3, "192.168.0.5", 3);
	if (ret < 0) {
		return ret;
	}

	dsa_register_recv_callback(ud.lan1, dsa_processing_cb);

	return 0;
}

void main(void)
{
	NET_INFO("DSA example!\n");

	init_dsa_ports();
}
