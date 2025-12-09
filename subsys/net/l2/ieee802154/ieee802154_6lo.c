/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 802.15.4 6LoWPAN adaptation layer implementation
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_6lo, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include "ieee802154_6lo.h"
#include "ieee802154_frame.h"
#include "ieee802154_security.h"

#include <6lo.h>
#include <ipv6.h>
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
#include "ieee802154_6lo_fragment.h"
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */

enum net_verdict ieee802154_6lo_decode_pkt(struct net_if *iface, struct net_pkt *pkt)
{
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	return ieee802154_6lo_reassemble(pkt);
#else
	if (!net_6lo_uncompress(pkt)) {
		NET_DBG("Packet decompression failed");
		return NET_DROP;
	}
	return NET_CONTINUE;
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */
}

int ieee802154_6lo_encode_pkt(struct net_if *iface, struct net_pkt *pkt,
			      struct ieee802154_6lo_fragment_ctx *frag_ctx, uint8_t ll_hdr_len,
			      uint8_t authtag_len)
{
	if (net_pkt_family(pkt) != NET_AF_INET6) {
		return 0;
	}

	/* hdr_diff will hold the hdr size difference on success */
	int hdr_diff = net_6lo_compress(pkt, true);

	if (hdr_diff < 0) {
		return hdr_diff;
	}

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	bool requires_fragmentation =
		ieee802154_6lo_requires_fragmentation(pkt, ll_hdr_len, authtag_len);

	if (requires_fragmentation) {
		ieee802154_6lo_fragment_ctx_init(frag_ctx, pkt, hdr_diff, true);
	}
	return requires_fragmentation ? 1 : 0;
#else
	return 0;
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */
}
