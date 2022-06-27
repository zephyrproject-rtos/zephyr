/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npf_ethernet, CONFIG_NET_PKT_FILTER_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>

static bool addr_mask_compare(struct net_eth_addr *addr1,
			      struct net_eth_addr *addr2,
			      struct net_eth_addr *mask)
{
	for (int i = 0; i < 6; i++) {
		if ((addr1->addr[i] & mask->addr[i]) !=
		    (addr2->addr[i] & mask->addr[i])) {
			return false;
		}
	}
	return true;
}

static bool addr_match(struct npf_test *test, struct net_eth_addr *pkt_addr)
{
	struct npf_test_eth_addr *test_eth_addr =
			CONTAINER_OF(test, struct npf_test_eth_addr, test);
	struct net_eth_addr *addr = test_eth_addr->addresses;
	struct net_eth_addr *mask = &test_eth_addr->mask;
	unsigned int nb_addr = test_eth_addr->nb_addresses;

	while (nb_addr) {
		if (addr_mask_compare(addr, pkt_addr, mask)) {
			return true;
		}
		addr++;
		nb_addr--;
	}
	return false;
}

bool npf_eth_src_addr_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct net_eth_hdr *eth_hdr = NET_ETH_HDR(pkt);

	return addr_match(test, &eth_hdr->src);
}

bool npf_eth_src_addr_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_eth_src_addr_match(test, pkt);
}

bool npf_eth_dst_addr_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct net_eth_hdr *eth_hdr = NET_ETH_HDR(pkt);

	return addr_match(test, &eth_hdr->dst);
}

bool npf_eth_dst_addr_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_eth_dst_addr_match(test, pkt);
}

bool npf_eth_type_match(struct npf_test *test, struct net_pkt *pkt)
{
	struct npf_test_eth_type *test_eth_type =
			CONTAINER_OF(test, struct npf_test_eth_type, test);
	struct net_eth_hdr *eth_hdr = NET_ETH_HDR(pkt);

	/* note: type_match->type is assumed to be in network order already */
	return eth_hdr->type == test_eth_type->type;
}

bool npf_eth_type_unmatch(struct npf_test *test, struct net_pkt *pkt)
{
	return !npf_eth_type_match(test, pkt);
}
