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

void ppp_network_up(struct ppp_context *ctx, int proto)
{
	if (ctx->network_protos_up == 0) {
		ppp_change_phase(ctx, PPP_RUNNING);
	}

	ctx->network_protos_up++;

	NET_DBG("[%p] Proto %s (0x%04x) %s (%d)", ctx, ppp_proto2str(proto),
		proto, "up", ctx->network_protos_up);
}

void ppp_network_down(struct ppp_context *ctx, int proto)
{
	ctx->network_protos_up--;

	if (ctx->network_protos_up <= 0) {
		ctx->network_protos_up = 0;
		ppp_change_phase(ctx, PPP_TERMINATE);
	}

	NET_DBG("[%p] Proto %s (0x%04x) %s (%d)", ctx, ppp_proto2str(proto),
		proto, "down", ctx->network_protos_up);
}

void ppp_network_done(struct ppp_context *ctx, int proto)
{
	ctx->network_protos_up--;
	if (ctx->network_protos_up <= 0) {
		const struct ppp_protocol_handler *proto = ppp_lcp_get();

		if (proto) {
			proto->close(ctx, "All networks down");
		}
	}
}

void ppp_network_all_down(struct ppp_context *ctx)
{
	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
		if (proto->protocol != PPP_LCP && proto->lower_down) {
			proto->lower_down(ctx);
		}

		if (proto->protocol < 0xC000 && proto->close) {
			ctx->network_protos_open--;
			proto->close(ctx, "LCP down");
		}
	}

	if (ctx->network_protos_open > 0) {
		NET_WARN("Not all network protocols were closed (%d)",
			 ctx->network_protos_open);
	}

	ctx->network_protos_open = 0;
}
