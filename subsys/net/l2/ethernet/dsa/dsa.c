/** @file
 * @brief LLDP related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dsa);

#include <errno.h>
#include <stdlib.h>

#include <net/net_core.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include <net/dsa.h>

/*
 * RECEIVE HANDLING CODE - ingress (ETH -> DSA slave ports)
 */

static int dsa_check_iface(struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -ENOENT;
	}

	if (!((net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) ||
	    (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_SLAVE_PORT))) {
		return -ESRCH;
	}

	return 0;
}


int dsa_register_recv_callback(struct net_if *iface, net_dsa_recv_cb_t cb)
{
	struct ethernet_context *ctx;
	int ret;

	ret = dsa_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	ctx = net_if_l2_data(iface);

	if (cb) {
		ctx->cb = cb;
	}

	return 0;
}

enum net_verdict net_dsa_recv(struct net_if *iface, struct net_pkt *pkt)
{

	struct ethernet_context *ctx;
	int ret;

	ret = dsa_check_iface(iface);
	if (ret < 0) {
		return NET_DROP;
	}

	ctx = net_if_l2_data(iface);

	if (ctx->cb) {
		return ctx->cb(iface, pkt);
	}

	return NET_CONTINUE;
}

int dsa_enable_port(struct net_if *iface, u8_t port)
{
	return 0;
}

__weak struct net_pkt *dsa_xmit_pkt(struct device *dev, struct net_pkt *pkt)
{
	return pkt;
}

__weak struct net_if *dsa_get_iface(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_linkaddr lladst;
	struct net_if *iface_sw;

	lladst.len = sizeof(hdr->dst.addr);
	lladst.addr = &hdr->dst.addr[0];

	iface_sw = net_if_get_by_link_addr(&lladst);
	if (iface_sw) {
		return iface_sw;
	}

	return iface;
}

struct net_if *dsa_recv_set_iface(struct net_if *iface, struct net_pkt **pkt)
{
	struct net_if *iface_sw = dsa_get_iface(iface, *pkt);

	/*
	 * Optional code to change the destination interface with some
	 * custome code.
	 */

	return iface_sw;
}

/*
 * TRANSMISSION HANDLING CODE egress (DSA slave ports -> ETH)
 */

int dsa_tx(struct device *dev, struct net_pkt *pkt)
{
	struct dsa_context *context = dev->driver_data;
	struct net_if *iface_master = context->iface_master;
	const struct ethernet_api *api =
		net_if_get_device(iface_master)->driver_api;

	if (iface_master == NULL) {
		NET_ERR("DSA: No master interface!");
		return -ENODEV;
	}

	/* Adjust packet for DSA routing and send it via master interface */
	return api->send(net_if_get_device(iface_master),
			 dsa_xmit_pkt(dev, pkt));
}
