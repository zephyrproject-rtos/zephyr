/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_DSA_TAG_NETC_H_
#define ZEPHYR_SUBSYS_DSA_TAG_NETC_H_

struct dsa_tag_netc_data {
#ifdef CONFIG_NET_L2_PTP
	void (*twostep_timestamp_handler)(const struct dsa_switch_context *ctx,
		uint8_t ts_req_id, uint64_t ts);
#endif
};

struct net_if *dsa_tag_netc_recv(struct net_if *iface, struct net_pkt *pkt);
struct net_pkt *dsa_tag_netc_xmit(struct net_if *iface, struct net_pkt *pkt);
#endif /* ZEPHYR_SUBSYS_DSA_TAG_NETC_H_ */
