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
#define MAC_CMD_TX_PARAM_SETUP_ANS_LEN	1

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

/*
 * EIRP a TxParamSetupReq's MaxEIRP index stands for, in dBm
 * (LoRaWAN TS001 MaxEIRP table).
 */
static const int8_t mac_cmd_max_eirp_dbm[] = {
	8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36,
};

#define MAC_CMD_TX_PARAM_DL_DWELL	BIT(5)
#define MAC_CMD_TX_PARAM_UL_DWELL	BIT(4)
#define MAC_CMD_TX_PARAM_EIRP_MASK	0x0fU

static void mac_cmd_handle_tx_param_setup(struct lwan_ctx *ctx, uint8_t param)
{
	uint8_t min_dr;

	if (ctx->region->get_dwell_tx_params == NULL) {
		/* The region caps nothing, so it does not take this command
		 * and does not answer it either.
		 */
		LOG_DBG("TxParamSetupReq ignored, region has no dwell time");
		return;
	}

	ctx->mac.dl_dwell_time = (param & MAC_CMD_TX_PARAM_DL_DWELL) != 0U;
	ctx->mac.ul_dwell_time = (param & MAC_CMD_TX_PARAM_UL_DWELL) != 0U;
	ctx->mac.max_eirp_dbm = mac_cmd_max_eirp_dbm[param & MAC_CMD_TX_PARAM_EIRP_MASK];
	ctx->mac.tx_param_setup_ans_pending = true;

	/*
	 * Taking the cap on leaves the slowest datarates without a payload,
	 * so a session sitting on one of them has to move up before the next
	 * uplink. Taking the cap off only widens the choice.
	 */
	min_dr = lwan_region_min_dr(ctx);
	if ((uint8_t)ctx->current_dr < min_dr) {
		LOG_INF("Datarate raised to DR%u by the dwell time limit", min_dr);
		ctx->current_dr = (enum lorawan_datarate)min_dr;
	}

	LOG_INF("TxParamSetup: ul_dwell=%u dl_dwell=%u max_eirp=%d dBm",
		ctx->mac.ul_dwell_time, ctx->mac.dl_dwell_time, ctx->mac.max_eirp_dbm);
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
		case MAC_CMD_TX_PARAM_SETUP:
			mac_cmd_handle_tx_param_setup(ctx, fopts[pos + 1]);
			break;
		default:
			LOG_DBG("Unhandled DL MAC command 0x%02X", cid);
			break;
		}

		pos += 1 + info->payload_len;
	}
}

size_t mac_cmd_next_ul_fopts_len(const struct lwan_ctx *ctx)
{
	size_t pos = 0;

	if (ctx->mac.tx_param_setup_ans_pending &&
	    pos + MAC_CMD_TX_PARAM_SETUP_ANS_LEN <= LWAN_MAX_FOPTS_LEN) {
		pos += MAC_CMD_TX_PARAM_SETUP_ANS_LEN;
	}

	if (ctx->mac.link_check_pending &&
	    pos + MAC_CMD_LINK_CHECK_REQ_LEN <= LWAN_MAX_FOPTS_LEN) {
		pos += MAC_CMD_LINK_CHECK_REQ_LEN;
	}

	return pos;
}

uint8_t mac_cmd_next_payload_size(const struct lwan_ctx *ctx,
				  uint8_t max_payload)
{
	size_t fopts_len = mac_cmd_next_ul_fopts_len(ctx);

	if (fopts_len >= max_payload) {
		return 0;
	}

	return (uint8_t)(max_payload - fopts_len);
}

size_t mac_cmd_build_ul_fopts(struct lwan_ctx *ctx,
			      uint8_t *buf, size_t max_len)
{
	size_t pos = 0;

	/* Reset the snapshot: the previous frame's emit state is now stale. */
	ctx->mac.ul_built_link_check_req = false;

	/* An answer the server is waiting for goes out ahead of a request. */
	if (ctx->mac.tx_param_setup_ans_pending &&
	    pos + MAC_CMD_TX_PARAM_SETUP_ANS_LEN <= max_len) {
		buf[pos++] = MAC_CMD_TX_PARAM_SETUP;
	}

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

void mac_cmd_dl_received(struct lwan_ctx *ctx)
{
	/*
	 * TS001 5.8: a TXParamSetupAns rides every uplink until a Class A
	 * downlink arrives, so that a lost uplink cannot leave the network
	 * guessing which parameters the device is on.
	 */
	ctx->mac.tx_param_setup_ans_pending = false;
}
