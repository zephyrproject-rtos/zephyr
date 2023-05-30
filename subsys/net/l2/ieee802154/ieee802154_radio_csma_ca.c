/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_csma, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154.h>

#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>

#include <stdlib.h>
#include <errno.h>

#include "ieee802154_frame.h"
#include "ieee802154_utils.h"
#include "ieee802154_radio_utils.h"

BUILD_ASSERT(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE <=
		     CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE,
	     "The CSMA/CA min backoff exponent must be less or equal max backoff exponent.");

/* See section 6.2.5.1. */
static inline int unslotted_csma_ca_radio_send(struct net_if *iface, struct net_pkt *pkt,
					       struct net_buf *frag)
{
	uint8_t be = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint32_t symbol_period, turnaround_time;
	uint8_t remaining_attempts;
	bool ack_required;
	bool is_subg_phy;
	uint8_t nb = 0U;
	int ret;

	NET_DBG("frag %p", frag);

	is_subg_phy = ieee802154_get_hw_capabilities(iface) & IEEE802154_HW_SUB_GHZ;
	/* TODO: Move symbol period calculation to radio driver. */
	symbol_period = IEEE802154_PHY_SYMBOL_PERIOD(is_subg_phy);
	turnaround_time = IEEE802154_PHY_A_TURNAROUND_TIME(is_subg_phy);

	/* See section 6.7.4.4 - Retransmissions. */
	remaining_attempts = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES + 1;

	while (remaining_attempts) {
		remaining_attempts--;

		while (1) {
			if (be) {
				uint8_t bo_n = sys_rand32_get() & ((1 << be) - 1);

				/* TODO: k_busy_wait() is too inaccurate on many platforms, the
				 * radio API should expose a precise radio clock instead (which may
				 * fall back to k_busy_wait() if the radio does not have a clock).
				 */
				k_busy_wait(bo_n * IEEE802154_A_UNIT_BACKOFF_PERIOD_US(
							   turnaround_time, symbol_period));
			}

			ret = ieee802154_cca(iface);
			if (ret == 0) {
				/* Channel is idle -> CSMA Success */
				break;
			} else if (ret != -EBUSY) {
				/* CCA exited with failure code. */
				return -EIO;
			}

			/* Channel is not idle -> CSMA Backoff */
			be = MIN(be + 1, CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE);
			nb++;

			if (nb > CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BO) {
				return -EIO;
			}
		}

		ack_required = prepare_for_ack(ctx, pkt, frag);

		if (!ack_required) {
			/* See section 6.7.4.4: "A device that sends a frame with the AR field set
			 * to indicate no acknowledgment requested may assume that the transmission
			 * was successfully received and shall not perform the retransmission
			 * procedure."
			 */
			remaining_attempts = 0;
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
FUNC_ALIAS(unslotted_csma_ca_radio_send,
	   ieee802154_radio_send, int);

FUNC_ALIAS(csma_ca_radio_handle_ack,
	   ieee802154_radio_handle_ack, enum net_verdict);
