/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include "lorawan.h"
#include "mac_commands.h"

/* MAC command identifiers (CID) */
#define MAC_CMD_LINK_CHECK	0x02

/* LinkCheckReq on-the-wire length: CID only, no payload */
#define MAC_CMD_LINK_CHECK_REQ_LEN	1

size_t mac_cmd_build_ul_fopts(struct lwan_ctx *ctx,
			      uint8_t *buf, size_t max_len)
{
	size_t pos = 0;

	/* Reset the snapshot — the previous frame's emit state is now stale. */
	ctx->mac.ul_built_link_check_req = false;

	if (ctx->mac.link_check_pending &&
	    pos + MAC_CMD_LINK_CHECK_REQ_LEN <= max_len) {
		buf[pos++] = MAC_CMD_LINK_CHECK;
		ctx->mac.ul_built_link_check_req = true;
	}

	return pos;
}

void mac_cmd_commit_ul_fopts(struct lwan_ctx *ctx)
{
	if (ctx->mac.ul_built_link_check_req) {
		ctx->mac.link_check_pending = false;
		ctx->mac.ul_built_link_check_req = false;
	}
}
