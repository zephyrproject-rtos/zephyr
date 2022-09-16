/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/net/ppp.h>

#include "net_private.h"

#include "ppp_internal.h"

static void lcp_up(struct ppp_context *ctx)
{
	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
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
	ppp_change_phase(ctx, PPP_NETWORK);

	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
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

	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
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
		const struct ppp_protocol_handler *proto = ppp_lcp_get();

		if (proto) {
			proto->close(ctx, "No network protocols open");
		}
	}
}

static void do_auth(struct ppp_context *ctx)
{
	uint16_t auth_proto = 0;

	ppp_change_phase(ctx, PPP_AUTH);

	if (IS_ENABLED(CONFIG_NET_L2_PPP_AUTH_SUPPORT)) {
		auth_proto = ctx->lcp.peer_options.auth_proto;
	}

	/* If no authentication is need, then we are done */
	if (!auth_proto) {
		ppp_link_authenticated(ctx);
		return;
	}

	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
		if (proto->protocol == auth_proto) {
			if (proto->open) {
				proto->open(ctx);
			}

			break;
		}
	}
}

void ppp_link_established(struct ppp_context *ctx, struct ppp_fsm *fsm)
{
	NET_DBG("[%p] Link established", ctx);

	ppp_change_phase(ctx, PPP_ESTABLISH);

	do_auth(ctx);

	lcp_up(ctx);
}

void ppp_link_authenticated(struct ppp_context *ctx)
{
	NET_DBG("[%p] Link authenticated", ctx);

	do_network(ctx);
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

	ppp_network_all_down(ctx);

	ppp_change_phase(ctx, PPP_DEAD);
}

void ppp_link_needed(struct ppp_context *ctx)
{
	/* TODO: Try to create link if needed. */
}
