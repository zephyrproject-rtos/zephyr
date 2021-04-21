/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_csma, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_if.h>

#include <sys/util.h>
#include <random/rand32.h>

#include <stdlib.h>
#include <errno.h>

#include "ieee802154_frame.h"
#include "ieee802154_utils.h"
#include "ieee802154_radio_utils.h"

static inline int csma_ca_radio_send(struct net_if *iface,
				     struct net_pkt *pkt,
				     struct net_buf *frag)
{
	const uint8_t max_bo = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BO;
	const uint8_t max_be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE;
	uint8_t retries = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	bool ack_required = prepare_for_ack(ctx, pkt, frag);
	uint8_t be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE;
	uint8_t nb = 0U;
	int ret = -EIO;

	NET_DBG("frag %p", frag);

loop:
	while (retries) {
		retries--;

		if (be) {
			uint8_t bo_n = sys_rand32_get() & ((1 << be) - 1);

			k_busy_wait(bo_n * 20U);
		}

		while (1) {
			if (!ieee802154_cca(iface)) {
				break;
			}

			be = MIN(be + 1, max_be);
			nb++;

			if (nb > max_bo) {
				goto loop;
			}
		}

		ret = ieee802154_tx(iface, IEEE802154_TX_MODE_DIRECT,
				    pkt, frag);
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

static enum net_verdict csma_ca_radio_handle_ack(struct net_if *iface,
						 struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA) &&
	    ieee802154_get_hw_capabilities(iface) & IEEE802154_HW_CSMA) {
		return NET_OK;
	}

	return handle_ack(ctx, pkt);
}

/* Declare the public Radio driver function used by the HW drivers */
FUNC_ALIAS(csma_ca_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(csma_ca_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
