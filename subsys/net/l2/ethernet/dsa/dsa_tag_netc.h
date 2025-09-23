/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_DSA_TAG_NETC_H_
#define ZEPHYR_SUBSYS_DSA_TAG_NETC_H_

struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt);
struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt);
#endif /* ZEPHYR_SUBSYS_DSA_TAG_NETC_H_ */
