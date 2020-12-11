/*
 * Copyright (c) 2019-2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_pkt.h>

#include "ppp_internal.h"

static enum net_verdict pap_handle(struct ppp_context *ctx,
				   struct net_if *iface,
				   struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->pap.fsm, PPP_PAP, pkt);
}

static struct net_pkt *pap_config_info_add(struct ppp_fsm *fsm)
{
	uint8_t payload[] = { 5, 'b', 'l', 'a', 'n', 'k',
			      5, 'b', 'l', 'a', 'n', 'k' };
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(ppp_fsm_iface(fsm), sizeof(payload),
					AF_UNSPEC, 0, PPP_BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	(void)net_pkt_write(pkt, payload, sizeof(payload));

	return pkt;
}

static int pap_config_info_ack(struct ppp_fsm *fsm,
			       struct net_pkt *pkt,
			       uint16_t length)
{
	/*
	 * We only support one way negotiation for now, so move to ACK_SENT
	 * phase right away.
	 */
	if (fsm->state == PPP_REQUEST_SENT) {
		ppp_change_state(fsm, PPP_ACK_SENT);
	}

	return 0;
}

static void pap_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->pap.fsm);
}

static void pap_lower_up(struct ppp_context *ctx)
{
	ppp_fsm_lower_up(&ctx->pap.fsm);
}

static void pap_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->pap.fsm);
}

static void pap_close(struct ppp_context *ctx, const uint8_t *reason)
{
	ppp_fsm_close(&ctx->pap.fsm, reason);
}

static void pap_up(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       pap.fsm);

	if (ctx->is_pap_up) {
		return;
	}

	ctx->is_pap_up = true;

	NET_DBG("[%s/%p] Current state %s (%d)", fsm->name, fsm,
		ppp_state_str(fsm->state), fsm->state);

	ppp_link_authenticated(ctx);
}

static void pap_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       pap.fsm);

	if (!ctx->is_pap_up) {
		return;
	}

	ctx->is_pap_up = false;
}

static void pap_finished(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       pap.fsm);

	if (!ctx->is_pap_open) {
		return;
	}

	ctx->is_pap_open = false;
}

static void pap_proto_reject(struct ppp_fsm *fsm)
{
	ppp_fsm_lower_down(fsm);
}

static void pap_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_PAP), PPP_PAP,
		&ctx->pap.fsm);

	memset(&ctx->pap.fsm, 0, sizeof(ctx->pap.fsm));

	ppp_fsm_init(&ctx->pap.fsm, PPP_PAP);

	ppp_fsm_name_set(&ctx->pap.fsm, ppp_proto2str(PPP_PAP));

	ctx->pap.fsm.cb.up = pap_up;
	ctx->pap.fsm.cb.down = pap_down;
	ctx->pap.fsm.cb.finished = pap_finished;
	ctx->pap.fsm.cb.proto_reject = pap_proto_reject;
	ctx->pap.fsm.cb.config_info_add = pap_config_info_add;
	ctx->pap.fsm.cb.config_info_ack = pap_config_info_ack;
}

PPP_PROTOCOL_REGISTER(PAP, PPP_PAP,
		      pap_init, pap_handle,
		      pap_lower_up, pap_lower_down,
		      pap_open, pap_close);
