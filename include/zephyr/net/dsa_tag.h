/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for DSA tag protocol
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_TAG_H_
#define ZEPHYR_INCLUDE_NET_DSA_TAG_H_

/**
 * @brief Definitions for DSA tag protocol
 * @defgroup dsa_tag DSA tag protocol
 * @since 4.4
 * @version 0.8.0
 * @ingroup ethernet
 * @{
 */

#include <zephyr/dt-bindings/ethernet/dsa_tag_proto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registration information for a dsa tag protocol
 */
struct dsa_tag_register {
	/** Protocol ID */
	int proto;

	/** Received packet handler */
	struct net_if *(*recv)(struct net_if *iface, struct net_pkt *pkt);

	/** Transmit packet handler */
	struct net_pkt *(*xmit)(struct net_if *iface, struct net_pkt *pkt);
};

#define DSA_TAG_REGISTER(_proto, _recv, _xmit)                                                     \
	static const STRUCT_SECTION_ITERABLE(dsa_tag_register, __dsa_tag_register_##_proto) = {    \
		.proto = _proto,                                                                   \
		.recv = _recv,                                                                     \
		.xmit = _xmit,                                                                     \
	}

/** @cond INTERNAL_HIDDEN */

/*
 * DSA tag protocol handles packet received by untagging
 *
 * param iface: Interface of DSA conduit port on which receives the packet
 * param pkt: Network packet
 *
 * Returns: Interface of DSA user port to redirect
 */
struct net_if *dsa_tag_recv(struct net_if *iface, struct net_pkt *pkt);

/*
 * DSA tag protocol handles packet transmitted by tagging
 *
 * param iface: Interface of DSA user port to transmit
 * param pkt: Network packet
 *
 * Returns: Network packet tagged
 */
struct net_pkt *dsa_tag_xmit(struct net_if *iface, struct net_pkt *pkt);

/*
 * Set up DSA tag protocol
 *
 * param dev_cpu: Device of DSA CPU port
 */
void dsa_tag_setup(const struct device *dev_cpu);

/** @endcond */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_DSA_TAG_H_ */
