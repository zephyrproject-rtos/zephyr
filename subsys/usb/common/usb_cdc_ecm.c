/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usb_cdc_ecm.h"

/* Retrieve expected pkt size from ethernet/ip header */
size_t ecm_eth_size(void *const ecm_pkt, const size_t len)
{
	uint8_t *ip_data = (uint8_t *)ecm_pkt + sizeof(struct net_eth_hdr);
	struct net_eth_hdr *hdr = (void *)ecm_pkt;
	uint16_t ip_len;

	if (len < NET_IPV6H_LEN + sizeof(struct net_eth_hdr)) {
		/* Too short */
		return 0;
	}

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
		__fallthrough;
	case NET_ETH_PTYPE_ARP:
		ip_len = ntohs(((struct net_ipv4_hdr *)ip_data)->len);
		break;
	case NET_ETH_PTYPE_IPV6:
		ip_len = ntohs(((struct net_ipv6_hdr *)ip_data)->len);
		break;
	default:
		return 0;
	}

	return sizeof(struct net_eth_hdr) + ip_len;
}
