/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 Private Management
 */

#ifndef __IEEE802154_MGMT_PRIV_H__
#define __IEEE802154_MGMT_PRIV_H__

#include "ieee802154_frame.h"

#ifdef CONFIG_NET_MGMT

static inline bool ieee802154_is_scanning(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return !(!ctx->scan_ctx);
}

static inline void ieee802154_mgmt_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	k_sem_init(&ctx->res_lock, 1, 1);
}

enum net_verdict ieee802154_handle_beacon(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu,
					  uint8_t lqi);

enum net_verdict ieee802154_handle_mac_command(struct net_if *iface,
					       struct ieee802154_mpdu *mpdu);

#else /* CONFIG_NET_MGMT */

#define ieee802154_is_scanning(...) false
#define ieee802154_mgmt_init(...)
#define ieee802154_handle_beacon(...) NET_DROP
#define ieee802154_handle_mac_command(...) NET_DROP

#endif /* CONFIG_NET_MGMT */

#endif /* __IEEE802154_MGMT_PRIV_H__ */
