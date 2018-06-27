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

#include <misc/util.h>

#include <stdlib.h>
#include <errno.h>

#include "ieee802154_frame.h"
#include "ieee802154_utils.h"
#include "ieee802154_radio_utils.h"

static inline int csma_ca_tx_fragment(struct net_if *iface,
				      struct net_pkt *pkt,
				      struct net_buf *frag)
{
	const u8_t max_bo = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BO;
	const u8_t max_be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE;
	u8_t retries = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	bool ack_required = prepare_for_ack(ctx, pkt, frag);
	u8_t be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE;
	u8_t nb = 0;
	int ret = -EIO;

	NET_DBG("frag %p", frag);

loop:
	while (retries) {
		retries--;

		if (be) {
			u8_t bo_n = sys_rand32_get() & ((1 << be) - 1);

			k_busy_wait(bo_n * 20);
		}

		while (1) {
			if (!ieee802154_cca(iface)) {
				break;
			}

			be = min(be + 1, max_be);
			nb++;

			if (nb > max_bo) {
				goto loop;
			}
		}

		ret = ieee802154_tx(iface, pkt, frag);
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

static int csma_ca_radio_send(struct net_if *iface, struct net_pkt *pkt)
{
	NET_DBG("pkt %p (frags %p)", pkt, pkt->frags);

	return tx_packet_fragments(iface, pkt, csma_ca_tx_fragment);
}

static enum net_verdict csma_ca_radio_handle_ack(struct net_if *iface,
						 struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return handle_ack(ctx, pkt);
}

/* Declare the public Radio driver function used by the HW drivers */
FUNC_ALIAS(csma_ca_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(csma_ca_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
