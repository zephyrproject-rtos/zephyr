/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_DSA_TAG_NETC_H_
#define ZEPHYR_SUBSYS_DSA_TAG_NETC_H_

#ifdef CONFIG_DSA_TAG_PROTOCOL_NETC
struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt);
struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt);
#else
static inline struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt)
{
	return iface;
}

static inline struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt)
{
	return pkt;
}
#endif /* CONFIG_DSA_TAG_PROTOCOL_NETC */

#endif /* ZEPHYR_SUBSYS_DSA_TAG_NETC_H_ */
