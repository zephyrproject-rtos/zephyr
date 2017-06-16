/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154)
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>
#include <net/net_if.h>

#include <errno.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"
#include "ieee802154_radio_utils.h"

static inline int aloha_tx_fragment(struct net_if *iface,
				    struct net_pkt *pkt,
				    struct net_buf *frag)
{
	u8_t retries = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	bool ack_required = prepare_for_ack(ctx, pkt, frag);
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;
	int ret = -EIO;

	NET_DBG("frag %p", frag);

	while (retries) {
		retries--;

		ret = radio->tx(iface->dev, pkt, frag);
		if (ret) {
			continue;
		}

		ret = wait_for_ack(iface, ack_required);
		if (!ret) {
			break;
		}
	}

	return ret;
}

static int aloha_radio_send(struct net_if *iface, struct net_pkt *pkt)
{
	NET_DBG("pkt %p (frags %p)", pkt, pkt->frags);

	return tx_packet_fragments(iface, pkt, aloha_tx_fragment);
}

static enum net_verdict aloha_radio_handle_ack(struct net_if *iface,
					       struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return handle_ack(ctx, pkt);
}

/* Declare the public Radio driver function used by the HW drivers */
FUNC_ALIAS(aloha_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(aloha_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
