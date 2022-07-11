/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/lldp.h>
#include <errno.h>

#include <zephyr/logging/log.h>
#include "main.h"

/* Loglevel of dsa_lldp function */
LOG_MODULE_DECLARE(net_dsa_lldp_sample, CONFIG_NET_DSA_LOG_LEVEL);

#define LLDP_SYSTEM_NAME_SIZE 24
#define LLDP_ETHER_TYPE 0x88CC
#define LLDP_INPUT_DATA_BUF_SIZE 512
#define DSA_BUF_SIZ 128
int dsa_lldp_send(struct net_if *iface, struct instance_data *pd,
		  uint16_t lan, int src_port, int origin_port, int cmd,
		  struct eth_addr *origin_addr)
{
	int ret, len;
	char buffer[DSA_BUF_SIZ], sys_name[LLDP_SYSTEM_NAME_SIZE];
	struct sockaddr_ll dst;
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_eth_hdr *eth_hdr = (struct net_eth_hdr *) buffer;
	uint8_t *p = &buffer[sizeof(struct net_eth_hdr)];
	uint8_t *pb = p;

	lan = ctx->dsa_port_idx;

	dst.sll_ifindex = net_if_get_by_iface(iface);
	/* Construct the Ethernet header */
	memset(buffer, 0, DSA_BUF_SIZ);
	/* Ethernet header */
	/* Take MAC address assigned to LAN port */
	memcpy(eth_hdr->src.addr, net_if_get_link_addr(iface)->addr, ETH_ALEN);
	eth_hdr->dst.addr[0] = MCAST_DEST_MAC0;
	eth_hdr->dst.addr[1] = MCAST_DEST_MAC1;
	eth_hdr->dst.addr[2] = MCAST_DEST_MAC2;
	eth_hdr->dst.addr[3] = MCAST_DEST_MAC3;
	eth_hdr->dst.addr[4] = MCAST_DEST_MAC4;
	eth_hdr->dst.addr[5] = MCAST_DEST_MAC5;

	/* Ethertype field */
	eth_hdr->type = htons(LLDP_ETHER_TYPE);

	/* LLDP packet data */
	/* Chassis ID */
	dsa_buf_write_be16((LLDP_TLV_CHASSIS_ID << 9) | (ETH_ALEN + 1), &p);
	*p++ = 4; /* subtype */
	memcpy(p, net_if_get_link_addr(iface)->addr, ETH_ALEN);
	p += ETH_ALEN;

	/* PORT ID */
	dsa_buf_write_be16((LLDP_TLV_PORT_ID << 9) | (ETH_ALEN + 1), &p);
	*p++ = 3; /* subtype */
	memcpy(p, net_if_get_link_addr(iface)->addr, ETH_ALEN);
	p += ETH_ALEN;

	/* TTL ID */
	dsa_buf_write_be16((LLDP_TLV_TTL << 9) | 2, &p);
	*p++ = 0; /* TTL field is 2 bytes long */
	*p++ = 120;

	/* SYSTEM NAME */
	memset(sys_name, 0, sizeof(sys_name));
	sprintf(sys_name, "ip_k66f LAN:%d", lan);
	dsa_buf_write_be16((LLDP_TLV_SYSTEM_NAME << 9) | strlen(sys_name), &p);
	memcpy(p, sys_name, strlen(sys_name));
	p += strlen(sys_name);

	len = sizeof(struct net_eth_hdr) + (p - pb);
	ret = sendto(pd->sock, buffer, len, 0, (const struct sockaddr *)&dst,
		     sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Failed to send, errno %d", errno);
	}

	return 0;
}

void dsa_lldp_print_info(uint8_t *lldp_p, uint8_t lanid)
{
	uint16_t tl, length;
	uint8_t type, subtype;
	uint8_t *p, t1, t2;
	char t[LLDP_INPUT_DATA_BUF_SIZE];

	LOG_INF("LLDP pkt recv -> lan%d", lanid);
	do {
		/* In-buffer data is stored as big endian */
		t1 = *lldp_p++;
		t2 = *lldp_p++;
		tl = (uint16_t) t1 << 8 | t2;

		/* Get type and length */
		type = tl >> 9;
		length = tl & 0x1FF;

		switch (type) {
		case LLDP_TLV_CHASSIS_ID:
		case LLDP_TLV_PORT_ID:
			/* Extract subtype */
			subtype = *lldp_p++;
			length--;
			break;
		}

		p = lldp_p;

		switch (type) {
		case LLDP_TLV_END_LLDPDU:
			return;
		case LLDP_TLV_CHASSIS_ID:
			LOG_INF("\tCHASSIS ID:\t%02x:%02x:%02x:%02x:%02x:%02x",
				p[0], p[1], p[2], p[3], p[4], p[5]);
			break;
		case LLDP_TLV_PORT_ID:
			LOG_INF("\tPORT ID:\t%02x:%02x:%02x:%02x:%02x:%02x",
				p[0], p[1], p[2], p[3], p[4], p[5]);
			break;
		case LLDP_TLV_TTL:
			/* TTL field has 2 bytes in BE */
			LOG_INF("\tTTL:\t\t%ds", (uint16_t) p[0] << 8 | p[1]);
			break;
		case LLDP_TLV_SYSTEM_NAME:
			memset(t, 0, length + 1);
			memcpy(t, p, length);
			LOG_INF("\tSYSTEM NAME:\t%s", t);
			break;
		}
		lldp_p += length;
	} while (1);
}

int dsa_lldp_recv(struct net_if *iface, struct instance_data *pd,
		  uint16_t *lan, int *origin_port,
		  struct eth_addr *origin_addr)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_eth_hdr *eth_hdr =
		(struct net_eth_hdr *) pd->recv_buffer;
	uint8_t *lldp_p = &pd->recv_buffer[sizeof(struct net_eth_hdr)];
	int received;

	*lan = ctx->dsa_port_idx;

	/* Receive data */
	received = recv(pd->sock, pd->recv_buffer,
			sizeof(pd->recv_buffer), 0);
	if (received < 0) {
		LOG_ERR("RAW : recv error %d", errno);
		return -1;
	}

	if (eth_hdr->type != 0xCC88) {
		LOG_ERR("Wrong LLDP packet type value [0x%x]", eth_hdr->type);
		return -1;
	}

	dsa_lldp_print_info(lldp_p, *lan);
	return 0;
}

DSA_THREAD(1, dsa_lldp_recv, dsa_lldp_send)
DSA_THREAD(2, dsa_lldp_recv, dsa_lldp_send)
DSA_THREAD(3, dsa_lldp_recv, dsa_lldp_send)
