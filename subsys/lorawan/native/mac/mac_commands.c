/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include <lorawan.h>
#include "mac_commands.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

/* LinkCheckReq on-the-wire length: CID only, no payload */
#define MAC_CMD_LINK_CHECK_REQ_LEN	1
#define MAC_CMD_LINK_ADR_REQ_LEN		5U
#define MAC_CMD_LINK_ADR_ANS_LEN		2U

#define LINK_ADR_CHANNEL_ACK	BIT(0)
#define LINK_ADR_DATARATE_ACK	BIT(1)
#define LINK_ADR_POWER_ACK	BIT(2)
#define LINK_ADR_ACK_ALL		GENMASK(2, 0)
#define LINK_ADR_KEEP_CURRENT	0x0fU

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
static lorawan_dr_changed_cb_t dr_changed_cb;

void mac_cmd_set_dr_changed_cb(lorawan_dr_changed_cb_t cb)
{
	dr_changed_cb = cb;
}

void mac_cmd_notify_dr_changed(enum lorawan_datarate dr)
{
	if (dr_changed_cb != NULL) {
		dr_changed_cb(dr);
	}
}

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

static bool mac_cmd_channel_supports_dr(const struct lwan_channel *channels, size_t count,
					uint8_t dr)
{
	for (size_t i = 0U; i < count; i++) {
		if (channels[i].enabled && channels[i].frequency != 0U &&
		    dr >= channels[i].min_dr && dr <= channels[i].max_dr) {
			return true;
		}
	}

	return false;
}

static void mac_cmd_handle_link_adr(struct lwan_ctx *ctx, const uint8_t *commands, size_t len)
{
	struct lwan_channel channels[LWAN_MAX_CHANNELS];
	const uint8_t *last = &commands[len - MAC_CMD_LINK_ADR_REQ_LEN];
	uint8_t dr = last[1] >> 4;
	uint8_t power = last[1] & 0x0fU;
	uint8_t nb_trans = last[4] & 0x0fU;
	uint8_t status = LINK_ADR_ACK_ALL;

	memcpy(channels, ctx->channels, sizeof(channels));
	for (size_t pos = 0U; pos < len; pos += MAC_CMD_LINK_ADR_REQ_LEN) {
		uint8_t control = (commands[pos + 4U] >> 4) & 0x07U;
		uint16_t mask = sys_get_le16(&commands[pos + 2U]);

		if (ctx->region->apply_adr_channel_mask(channels, ctx->channel_count, control,
							mask) != 0) {
			status &= ~LINK_ADR_CHANNEL_ACK;
		}
	}

	if (dr == LINK_ADR_KEEP_CURRENT) {
		dr = ctx->current_dr;
	}
	if (power == LINK_ADR_KEEP_CURRENT) {
		power = ctx->mac.tx_power_idx;
	}

	if (ctx->region->validate_dr(dr) != 0) {
		status &= ~LINK_ADR_DATARATE_ACK;
	}
	if (ctx->region->validate_tx_power(power) != 0) {
		status &= ~LINK_ADR_POWER_ACK;
	}

	if (!ctx->mac.adr_enabled) {
		/* The application retains control of DR, power and repetitions. */
		if (dr != ctx->current_dr) {
			status &= ~LINK_ADR_DATARATE_ACK;
		}
		if (power != ctx->mac.tx_power_idx) {
			status &= ~LINK_ADR_POWER_ACK;
		}
		dr = ctx->current_dr;
	}

	if (!mac_cmd_channel_supports_dr(channels, ctx->channel_count, dr)) {
		status &= ~(LINK_ADR_CHANNEL_ACK | LINK_ADR_DATARATE_ACK);
	}

	if (status == LINK_ADR_ACK_ALL ||
	    (!ctx->mac.adr_enabled && (status & LINK_ADR_CHANNEL_ACK) != 0U)) {
		memcpy(ctx->channels, channels, sizeof(channels));
		if (ctx->mac.adr_enabled) {
			ctx->current_dr = dr;
			ctx->mac.tx_power_idx = power;
			ctx->mac.nb_trans = MAX(nb_trans, 1U);
		}
	}

	for (size_t pos = 0U; pos < len; pos += MAC_CMD_LINK_ADR_REQ_LEN) {
		if (ctx->mac.answers_len + MAC_CMD_LINK_ADR_ANS_LEN > sizeof(ctx->mac.answers)) {
			break;
		}
		ctx->mac.answers[ctx->mac.answers_len++] = MAC_CMD_LINK_ADR;
		ctx->mac.answers[ctx->mac.answers_len++] = status;
	}
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
		case MAC_CMD_LINK_ADR: {
			size_t end = pos + MAC_CMD_LINK_ADR_REQ_LEN;

			while (end + MAC_CMD_LINK_ADR_REQ_LEN <= fopts_len &&
			       fopts[end] == MAC_CMD_LINK_ADR) {
				end += MAC_CMD_LINK_ADR_REQ_LEN;
			}
			mac_cmd_handle_link_adr(ctx, &fopts[pos], end - pos);
			pos = end;
			continue;
		}
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

size_t mac_cmd_next_ul_commands_len(const struct lwan_ctx *ctx)
{
	size_t pos = ctx->mac.answers_len;

	if (ctx->mac.link_check_pending) {
		pos += MAC_CMD_LINK_CHECK_REQ_LEN;
	}

	return pos;
}

uint8_t mac_cmd_next_payload_size(const struct lwan_ctx *ctx,
				  uint8_t max_payload)
{
	size_t fopts_len = mac_cmd_next_ul_commands_len(ctx);

	if (fopts_len >= max_payload) {
		return 0;
	}

	return fopts_len > LWAN_MAX_FOPTS_LEN ? 0U : (uint8_t)(max_payload - fopts_len);
}

size_t mac_cmd_build_ul_commands(struct lwan_ctx *ctx, uint8_t *buf, size_t max_len)
{
	size_t pos = 0;

	/* Reset the snapshot — the previous frame's emit state is now stale. */
	ctx->mac.ul_built_link_check_req = false;
	/* LoRaWAN 1.0.4 section 5 requires one response frame per answer batch.
	 * After successful TX, discard any tail truncated at the payload limit.
	 */
	ctx->mac.ul_built_answers_len = ctx->mac.answers_len;

	/* Truncate only at an answer boundary if the regional limit is smaller. */
	pos = MIN(ctx->mac.answers_len,
		  max_len / MAC_CMD_LINK_ADR_ANS_LEN * MAC_CMD_LINK_ADR_ANS_LEN);
	memcpy(buf, ctx->mac.answers, pos);

	if (pos == ctx->mac.answers_len && ctx->mac.link_check_pending &&
	    pos + MAC_CMD_LINK_CHECK_REQ_LEN <= max_len) {
		buf[pos++] = MAC_CMD_LINK_CHECK;
		ctx->mac.ul_built_link_check_req = true;
	}

	return pos;
}

void mac_cmd_commit_ul_commands(struct lwan_ctx *ctx)
{
	uint8_t consumed = ctx->mac.ul_built_answers_len;

	if (consumed > 0U) {
		ctx->mac.answers_len -= consumed;
		memmove(ctx->mac.answers, &ctx->mac.answers[consumed], ctx->mac.answers_len);
		ctx->mac.ul_built_answers_len = 0U;
	}

	if (ctx->mac.ul_built_link_check_req) {
		ctx->mac.link_check_pending = false;
		ctx->mac.ul_built_link_check_req = false;
	}
}
