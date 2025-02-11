/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_lldp_sample, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/dsa.h>
#include "main.h"

static void iface_cb(struct net_if *iface, void *user_data)
{

	struct ud *ifaces = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
		if (ifaces->master == NULL) {
			ifaces->master = iface;

			/* Get slave interfaces */
			for (int i = 0; i < ARRAY_SIZE(ifaces->lan); i++) {
				struct net_if *slave = dsa_get_slave_port(iface, i);

				if (slave == NULL) {
					LOG_ERR("Slave interface %d not found.", i);
					break;
				}

				ifaces->lan[i] = slave;
			}
			return;
		}
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

	pd->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
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

struct ud user_data_ifaces;
static int init_dsa_ports(void)
{
	uint8_t tbl_buf[8];

	/* Initialize interfaces - read them to user_data_ifaces */
	(void)memset(&user_data_ifaces, 0, sizeof(user_data_ifaces));
	net_if_foreach(iface_cb, &user_data_ifaces);

	/*
	 * Set static table to forward LLDP protocol packets
	 * to master port.
	 */
	dsa_switch_set_mac_table_entry(user_data_ifaces.lan[0],
					      &eth_filter_l2_addr_base[0][0],
					      BIT(4), 0, 0);
	dsa_switch_get_mac_table_entry(user_data_ifaces.lan[0], tbl_buf, 0);

	LOG_INF("DSA static MAC address table entry [%d]:", 0);
	LOG_INF("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		tbl_buf[7], tbl_buf[6], tbl_buf[5], tbl_buf[4],
		tbl_buf[3], tbl_buf[2], tbl_buf[1], tbl_buf[0]);

	return 0;
}

int main(void)
{
	init_dsa_ports();

	LOG_INF("DSA ports init - OK");
	return 0;
}
