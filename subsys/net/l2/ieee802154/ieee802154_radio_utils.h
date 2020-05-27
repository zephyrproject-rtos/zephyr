/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_RADIO_UTILS_H__
#define __IEEE802154_RADIO_UTILS_H__

#include "ieee802154_utils.h"

/**
 * @brief Radio driver sending function that radio drivers should implement
 *
 * @param iface A valid pointer on a network interface to send from
 * @param pkt A valid pointer on a packet to send
 *
 * @return 0 on success, negative value otherwise
 */
extern int ieee802154_radio_send(struct net_if *iface,
				 struct net_pkt *pkt,
				 struct net_buf *frag);

static inline bool prepare_for_ack(struct ieee802154_context *ctx,
				   struct net_pkt *pkt,
				   struct net_buf *frag)
{
	if (ieee802154_is_ar_flag_set(frag)) {
		struct ieee802154_fcf_seq *fs;

		fs = (struct ieee802154_fcf_seq *)frag->data;

		ctx->ack_seq = fs->sequence;
		ctx->ack_received = false;
		k_sem_init(&ctx->ack_lock, 0, UINT_MAX);

		return true;
	}

	return false;
}

static inline int wait_for_ack(struct net_if *iface,
			       bool ack_required)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);


	if (!ack_required ||
	    (ieee802154_get_hw_capabilities(iface) & IEEE802154_HW_TX_RX_ACK)) {
		return 0;
	}

	if (k_sem_take(&ctx->ack_lock, K_MSEC(10)) == 0) {
		/*
		 * We reinit the semaphore in case handle_ack
		 * got called multiple times.
		 */
		k_sem_init(&ctx->ack_lock, 0, UINT_MAX);
	}

	ctx->ack_seq = 0U;

	return ctx->ack_received ? 0 : -EIO;
}

static inline int handle_ack(struct ieee802154_context *ctx,
			     struct net_pkt *pkt)
{
	if (pkt->buffer->len == IEEE802154_ACK_PKT_LENGTH) {
		uint8_t len = IEEE802154_ACK_PKT_LENGTH;
		struct ieee802154_fcf_seq *fs;

		fs = ieee802154_validate_fc_seq(net_pkt_data(pkt), NULL, &len);
		if (!fs || fs->sequence != ctx->ack_seq) {
			return NET_CONTINUE;
		}

		ctx->ack_received = true;
		k_sem_give(&ctx->ack_lock);

		return NET_OK;
	}

	return NET_CONTINUE;
}

#endif /* __IEEE802154_RADIO_UTILS_H__ */
