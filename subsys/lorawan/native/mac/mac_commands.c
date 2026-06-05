/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <lorawan.h>
#include "mac_commands.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

/* LinkCheckReq on-the-wire length: CID only, no payload */
#define MAC_CMD_LINK_CHECK_REQ_LEN	1

/* Downlink command payload lengths in bytes, excluding the CID */
struct mac_dl_cmd_info {
	uint8_t cid;
	uint8_t payload_len;
};

static const struct mac_dl_cmd_info dl_cmd_table[] = {
	{ MAC_CMD_RESET,		1 },
	{ MAC_CMD_LINK_CHECK,		2 },	/* Margin + GwCnt */
	{ MAC_CMD_LINK_ADR,		4 },
	{ MAC_CMD_DUTY_CYCLE,		1 },
	{ MAC_CMD_RX_PARAM_SETUP,	4 },
	{ MAC_CMD_DEV_STATUS,		0 },
	{ MAC_CMD_NEW_CHANNEL,		5 },
	{ MAC_CMD_RX_TIMING_SETUP,	1 },
	{ MAC_CMD_TX_PARAM_SETUP,	1 },
	{ MAC_CMD_DL_CHANNEL,		4 },
	{ MAC_CMD_REKEY,		1 },
	{ MAC_CMD_ADR_PARAM_SETUP,	1 },
	{ MAC_CMD_DEVICE_TIME,		5 },
	{ MAC_CMD_FORCE_REJOIN,		2 },
	{ MAC_CMD_REJOIN_PARAM,		1 },
	{ MAC_CMD_PING_SLOT_INFO,	0 },	/* PingSlotInfoAns is empty */
	{ MAC_CMD_PING_SLOT_CHAN,	4 },
	{ MAC_CMD_BEACON_TIMING,	3 },
	{ MAC_CMD_BEACON_FREQ,		3 },
	{ MAC_CMD_DEVICE_MODE,		1 },
};

static lorawan_link_check_ans_cb_t link_check_cb;

void mac_cmd_set_link_check_cb(lorawan_link_check_ans_cb_t cb)
{
	link_check_cb = cb;
}

static void mac_cmd_handle_link_check_ans(struct lwan_ctx *ctx,
					  uint8_t margin, uint8_t gw_cnt)
{
	ctx->mac.link_check_margin = margin;
	ctx->mac.link_check_gw_cnt = gw_cnt;
	ctx->mac.link_check_ans_valid = true;

	LOG_INF("LinkCheckAns: margin=%u dB, gateways=%u", margin, gw_cnt);
}

void mac_cmd_deliver_link_check_ans(struct lwan_ctx *ctx)
{
	uint8_t margin;
	uint8_t gw_cnt;

	if (!ctx->mac.link_check_ans_valid) {
		return;
	}

	margin = ctx->mac.link_check_margin;
	gw_cnt = ctx->mac.link_check_gw_cnt;
	ctx->mac.link_check_ans_valid = false;

	if (link_check_cb != NULL) {
		link_check_cb(margin, gw_cnt);
	}
}

bool mac_cmd_has_pending_delivery(struct lwan_ctx *ctx)
{
	return ctx->mac.link_check_ans_valid;
}

static const struct mac_dl_cmd_info *mac_cmd_dl_info(uint8_t cid)
{
	for (size_t i = 0; i < ARRAY_SIZE(dl_cmd_table); i++) {
		if (dl_cmd_table[i].cid == cid) {
			return &dl_cmd_table[i];
		}
	}

	return NULL;
}

void mac_cmd_process_dl_fopts(struct lwan_ctx *ctx,
			      const uint8_t *fopts, size_t fopts_len)
{
	size_t pos = 0;

	while (pos < fopts_len) {
		uint8_t cid = fopts[pos];
		const struct mac_dl_cmd_info *info = mac_cmd_dl_info(cid);

		if (info == NULL) {
			/* Unknown CID: length unknown, cannot parse further */
			LOG_WRN("Unknown DL MAC command 0x%02X, dropping %zu byte(s)",
				cid, fopts_len - pos);
			return;
		}

		if (pos + 1 + info->payload_len > fopts_len) {
			LOG_WRN("Truncated DL MAC command 0x%02X", cid);
			return;
		}

		switch (cid) {
		case MAC_CMD_LINK_CHECK:
			mac_cmd_handle_link_check_ans(ctx, fopts[pos + 1],
						      fopts[pos + 2]);
			break;
		default:
			LOG_DBG("Unhandled DL MAC command 0x%02X", cid);
			break;
		}

		pos += 1 + info->payload_len;
	}
}

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
