/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/net_mgmt.h>

#include <net/ppp.h>

#include "net_private.h"

#include "ppp_stats.h"
#include "ppp_internal.h"

static enum net_verdict lcp_handle_ext(struct ppp_fsm *fsm,
				       enum ppp_packet_type code, uint8_t id,
				       struct net_pkt *pkt)
{
	enum net_verdict verdict = NET_DROP;

	switch (code) {
	case PPP_PROTOCOL_REJ:
		NET_DBG("PPP Protocol-Rej");
		return ppp_fsm_recv_protocol_rej(fsm, id, pkt);

	case PPP_ECHO_REQ:
		NET_DBG("PPP Echo-Req");
		return ppp_fsm_recv_echo_req(fsm, id, pkt);

	case PPP_ECHO_REPLY:
		NET_DBG("PPP Echo-Reply");
		return ppp_fsm_recv_echo_reply(fsm, id, pkt);

	case PPP_DISCARD_REQ:
		NET_DBG("PPP Discard-Req");
		return ppp_fsm_recv_discard_req(fsm, id, pkt);

	default:
		break;
	}

	return verdict;
}

static enum net_verdict lcp_handle(struct ppp_context *ctx,
				   struct net_if *iface,
				   struct net_pkt *pkt)
{
	return ppp_fsm_input(&ctx->lcp.fsm, PPP_LCP, pkt);
}

static void lcp_lower_down(struct ppp_context *ctx)
{
	ppp_fsm_lower_down(&ctx->lcp.fsm);
}

static void lcp_lower_up(struct ppp_context *ctx)
{
	ppp_fsm_lower_up(&ctx->lcp.fsm);
}

static void lcp_open(struct ppp_context *ctx)
{
	ppp_fsm_open(&ctx->lcp.fsm);
}

static void lcp_close(struct ppp_context *ctx, const uint8_t *reason)
{
	if (ctx->phase != PPP_DEAD) {
		ppp_change_phase(ctx, PPP_TERMINATE);
	}

	ppp_change_state(&ctx->lcp.fsm, PPP_STOPPED);

	ppp_fsm_close(&ctx->lcp.fsm, reason);
}

static void lcp_down(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_down(ctx);

	ppp_change_phase(ctx, PPP_ESTABLISH);
}

static void lcp_up(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	/* TODO: Set MRU/MTU of the network interface here */

	ppp_link_established(ctx, fsm);
}

static void lcp_starting(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_needed(ctx);
}

static void lcp_finished(struct ppp_fsm *fsm)
{
	struct ppp_context *ctx = CONTAINER_OF(fsm, struct ppp_context,
					       lcp.fsm);

	ppp_link_terminated(ctx);
}

static void lcp_init(struct ppp_context *ctx)
{
	NET_DBG("proto %s (0x%04x) fsm %p", ppp_proto2str(PPP_LCP), PPP_LCP,
		&ctx->lcp.fsm);

	memset(&ctx->lcp.fsm, 0, sizeof(ctx->lcp.fsm));

	ppp_fsm_init(&ctx->lcp.fsm, PPP_LCP);

	ppp_fsm_name_set(&ctx->lcp.fsm, ppp_proto2str(PPP_LCP));

	ctx->lcp.fsm.cb.up = lcp_up;
	ctx->lcp.fsm.cb.down = lcp_down;
	ctx->lcp.fsm.cb.starting = lcp_starting;
	ctx->lcp.fsm.cb.finished = lcp_finished;
	ctx->lcp.fsm.cb.proto_extension = lcp_handle_ext;
}

PPP_PROTOCOL_REGISTER(LCP, PPP_LCP,
		      lcp_init, lcp_handle,
		      lcp_lower_up, lcp_lower_down,
		      lcp_open, lcp_close);
