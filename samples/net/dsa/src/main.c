/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dsa_lldp_sample, CONFIG_NET_DSA_LOG_LEVEL);

#include <net/dsa.h>
#include "main.h"

static void dsa_slave_port_setup(struct net_if *iface, struct ud *ifaces)
{
	struct dsa_context *context = net_if_get_device(iface)->data;

	if (ifaces->lan1 == NULL) {
		ifaces->lan1 = context->iface_slave[1];
	}

	if (ifaces->lan2 == NULL) {
		ifaces->lan2 = context->iface_slave[2];
	}

	if (ifaces->lan3 == NULL) {
		ifaces->lan3 = context->iface_slave[3];
	}
}

static void iface_cb(struct net_if *iface, void *user_data)
{

	struct ud *ifaces = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
		if (ifaces->master == NULL) {
			ifaces->master = iface;
			return;
		}
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_SLAVE_PORT) {
		dsa_slave_port_setup(iface, ifaces);
	}
}

static const uint8_t eth_filter_l2_addr_base[][6] = {
	/* MAC address of other device - for filtering testing */
	{ 0x01, 0x80, 0xc2, 0x00, 0x00, 0x03 }
};

enum net_verdict dsa_ll_addr_switch_cb(struct net_if *iface,
				       struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_linkaddr lladst;

	net_pkt_cursor_init(pkt);
	lladst.len = sizeof(hdr->dst.addr);
	lladst.addr = &hdr->dst.addr[0];

	/*
	 * Pass packet to lan1..3 when matching one from
	 * check_ll_ether_addr table
	 */
	if (check_ll_ether_addr(lladst.addr, &eth_filter_l2_addr_base[0][0])) {
		return 1;
	}

	return 0;
}

int start_slave_port_packet_socket(struct net_if *iface,
				   struct instance_data *pd)
{
	struct sockaddr_ll dst;
	int ret;

	pd->sock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
	if (pd->sock < 0) {
		LOG_ERR("Failed to create RAW socket : %d", errno);
		return -errno;
	}

	dst.sll_ifindex = net_if_get_by_iface(iface);
	dst.sll_family = AF_PACKET;

	ret = bind(pd->sock, (const struct sockaddr *)&dst,
		   sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Failed to bind packet socket : %d", errno);
		return -errno;
	}

	return 0;
}

struct ud ifaces;
static int init_dsa_ports(void)
{
	struct dsa_context *ctx;
	uint8_t tbl_buf[8];

	/* Initialize interfaces - read them to ifaces */
	(void)memset(&ifaces, 0, sizeof(ifaces));
	net_if_foreach(iface_cb, &ifaces);

	/*
	 * Read the DSA context - single structure for all
	 * LAN ports.
	 */
	ctx = dsa_get_context(ifaces.lan1);
	if (ctx == NULL) {
		LOG_ERR("DSA context not available!");
		return -ENODEV;
	}

	/*
	 * Set static table to forward LLDP protocol packets
	 * to master port.
	 */
	ctx->dapi->switch_set_mac_table_entry(ctx->switch_id,
					      &eth_filter_l2_addr_base[0][0],
					      BIT(4), 0, 0);
	ctx->dapi->switch_get_mac_table_entry(ctx->switch_id, tbl_buf, 0);

	LOG_INF("DSA chip:%d static MAC address table entry [%d]:",
		ctx->switch_id, 0);
	LOG_INF("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		tbl_buf[7], tbl_buf[6], tbl_buf[5], tbl_buf[4],
		tbl_buf[3], tbl_buf[2], tbl_buf[1], tbl_buf[0]);

	return 0;
}

void main(void)
{
	init_dsa_ports();

	LOG_INF("DSA ports init - OK");
}
