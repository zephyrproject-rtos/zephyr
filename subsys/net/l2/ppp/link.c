/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_pkt.h>

#include <net/ppp.h>

#include "net_private.h"

#include "ppp_internal.h"

static void lcp_up(struct ppp_context *ctx)
{
	struct ppp_protocol_handler *proto;

	for (proto = __net_ppp_proto_start;
	     proto != __net_ppp_proto_end;
	     proto++) {
		if (proto->protocol == PPP_LCP) {
			continue;
		}

		if (proto->lower_up) {
			proto->lower_up(ctx);
		}
	}
}

static void do_network(struct ppp_context *ctx)
{
	const struct ppp_protocol_handler *proto;

	ppp_change_phase(ctx, PPP_NETWORK);

	for (proto = __net_ppp_proto_start;
	     proto != __net_ppp_proto_end;
	     proto++) {
		if (proto->protocol == PPP_CCP || proto->protocol == PPP_ECP) {
			if (proto->open) {
				proto->open(ctx);
			}
		}
	}

	/* Do the other network protocols if encryption is not needed for
	 * them.
	 */
	/* TODO possible encryption stuff here*/

	for (proto = __net_ppp_proto_start;
	     proto != __net_ppp_proto_end;
	     proto++) {
		if (proto->protocol == PPP_CCP || proto->protocol == PPP_ECP ||
		    proto->protocol >= 0xC000) {
			continue;
		}

		if (proto->open) {
			ctx->network_protos_open++;
			proto->open(ctx);
		}
	}

	if (ctx->network_protos_open == 0) {
		proto = ppp_lcp_get();
		if (proto) {
			proto->close(ctx, "No network protocols open");
		}
	}
}

void ppp_link_established(struct ppp_context *ctx, struct ppp_fsm *fsm)
{
	NET_DBG("[%p] Link established", ctx);

	ppp_change_phase(ctx, PPP_ESTABLISH);

	ppp_change_phase(ctx, PPP_AUTH);

	/* If no authentication is need, then we are done */
	/* TODO: check here if auth is needed */

	do_network(ctx);

	lcp_up(ctx);
}

void ppp_link_terminated(struct ppp_context *ctx)
{
	if (ctx->phase == PPP_DEAD) {
		return;
	}

	/* TODO: cleanup things etc here if needed */

	ppp_change_phase(ctx, PPP_DEAD);

	NET_DBG("[%p] Link terminated", ctx);
}

void ppp_link_down(struct ppp_context *ctx)
{
	if (ctx->phase == PPP_DEAD) {
		return;
	}

	ppp_change_phase(ctx, PPP_NETWORK);

	ppp_network_all_down(ctx);

	ppp_change_phase(ctx, PPP_DEAD);
}

void ppp_link_needed(struct ppp_context *ctx)
{
	/* TODO: Try to create link if needed. */
}
