/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_MSG_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_MSG_H_

#include <zephyr/device.h>

/*
 * NXP NETC VSI-to-PSI mailbox messaging.
 *
 * A VSI cannot write the PSI's shared MAC/filter/link registers, so it requests
 * those operations from the PSI owner over the VSI-to-PSI message channel. This
 * module owns that transport and the per-operation message encoders; the VSI
 * driver calls this API and keeps the interface/lifecycle policy.
 */

/* Size of the DMA-visible VSI->PSI message buffer. */
#define NETC_MSG_VSI_BUF_SIZE 64U

/* MAC filter type bits (match the ENETC VSI->PSI mailbox ABI). */
#define NETC_MAC_FILTER_TYPE_UC BIT(0)
#define NETC_MAC_FILTER_TYPE_MC BIT(1)

/* Link state parsed from a pending PSI->VSI message. */
enum netc_msg_link {
	NETC_MSG_LINK_NONE,
	NETC_MSG_LINK_UP,
	NETC_MSG_LINK_DOWN,
};

/* Ask the PSI to route this VSI's primary unicast MAC (data->mac_addr) to it. */
int netc_vsi_msg_set_primary_mac(const struct device *dev);

/* Program this VSI's 64-bit multicast hash table (data->mc_hash) in the PSI. */
int netc_vsi_msg_set_mac_hash(const struct device *dev);

#if defined(CONFIG_NET_VLAN)
/* Program this VSI's 64-bit VLAN hash table (data->vlan_hash) in the PSI. */
int netc_vsi_msg_set_vlan_hash(const struct device *dev);
#endif

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
/* Ask the PSI to set or clear UC/MC promiscuous mode for this VSI. */
int netc_vsi_msg_set_mac_promisc(const struct device *dev, uint8_t type_mask, bool enable);
#endif

/* Subscribe to PSI link-status notifications for this VSI. */
int netc_vsi_msg_enable_link_notify(const struct device *dev);

/* Consume a pending PSI->VSI message and report any link-state change. */
enum netc_msg_link netc_vsi_msg_poll_link(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_MSG_H_ */
