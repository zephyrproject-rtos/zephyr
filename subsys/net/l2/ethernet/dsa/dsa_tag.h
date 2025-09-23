/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_DSA_TAG_PRIV_H_
#define ZEPHYR_SUBSYS_DSA_TAG_PRIV_H_

#include <zephyr/dt-bindings/ethernet/dsa_tag_proto.h>
#ifdef CONFIG_DSA_TAG_PROTOCOL_NETC
#include "dsa_tag_netc.h"
#endif

struct net_if *dsa_tag_recv(struct net_if *iface, struct net_pkt *pkt);
struct net_pkt *dsa_tag_xmit(struct net_if *iface, struct net_pkt *pkt);
void dsa_tag_setup(const struct device *dev_cpu);

#endif
